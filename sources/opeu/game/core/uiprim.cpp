// UI primitives: pictures streamed from a file or from memory into the decoder, the portrait and face caches, the
// context frame, outlines and backdrops, the title box and the palettes for the time of day.
#include "core/uiprim.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/host.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstdint>
#include <utility>

using namespace opeu::game;

namespace {

// The services a UI primitive calls through its host's event.
enum class UiService : std::uint16_t {
  SEEK          = 0,  // (handle, position low, high, origin) -> DX:AX
  READ          = 1,  // (handle, segment, offset, count) -> bytes read
  BLIT          = 2,  // (x, y, w, h, mode, segment, offset)
  DRAW          = 3,  // (x, y, w, h, mode, segment, offset)
  DRAW_NEAR     = 4,  // (x, y, w, h, mode, near pointer)
  LINE          = 5,  // (x1, y1, x2, y2, color)
  BOX           = 6,  // (x, y, w, h, color)
  COPY          = 7,  // (x, y, w, h, x2, y2): a screen area to another place
  SET_PALETTE   = 8,  // (first, count, near pointer)
  DECODE        = 9,  // (x, y, reader offset, reader segment)
  DECODE_MASKED = 10, // (x, y, reader offset, reader segment)
  POSITION      = 11, // (x, y): the text cursor
};

// The decoder's byte readers in this module's code segment: opeu_stream_file_byte and opeu_stream_memory_byte.
constexpr std::uint16_t FILE_READER{ 0x132 };
constexpr std::uint16_t MEMORY_READER{ 0x1fc };

// The image buffer segment: pictures are read and decoded below this offset, faces unpacked at FACE_BUFFER.
constexpr std::uint16_t IMAGE_END{ 0xc800 };
constexpr std::uint16_t FACE_BUFFER{ 0xc080 };
constexpr std::uint16_t STREAM_BUFFER{ 0xa000 }; // where a file picture is buffered
constexpr std::uint16_t STREAM_CHUNK{ 0x2800 };

constexpr std::uint16_t IMAGE_BYTES{ 0x780 }; // a portrait or face: 24 x 80 pixels
constexpr std::uint8_t  END_OF_LIST{ 0xff };
constexpr std::int32_t  SEEK_FAILED{ -1 };

template <typename... Words>
auto UiCall(std::uint8_t* ds, UiHost const* host, UiService service, Words... words) -> std::uint32_t {
  std::uint16_t const args[]{ static_cast<std::uint16_t>(words)... };
  return host->event(ds, host->memory, std::to_underlying(service), args, sizeof...(Words));
}

auto Seek(std::uint8_t* ds, UiHost const* host, std::int16_t handle, std::int32_t position, std::uint16_t origin) -> std::int32_t {
  return static_cast<std::int32_t>(UiCall(ds, host, UiService::SEEK, handle, static_cast<std::uint16_t>(position), static_cast<std::uint16_t>(static_cast<std::uint32_t>(position) >> 16), origin));
}

auto ReadFar(std::uint8_t* ds, UiHost const* host, std::int16_t handle, std::uint16_t segment, std::uint16_t offset, std::uint16_t count) -> std::uint16_t {
  return static_cast<std::uint16_t>(UiCall(ds, host, UiService::READ, handle, segment, offset, count));
}

auto Multiply(std::uint16_t a, std::uint16_t b) -> std::uint16_t {
  return static_cast<std::uint16_t>(static_cast<std::uint32_t>(a) * b);
}

// The byte a far pointer addresses: in the data segment when it aliases it, otherwise in the host's flat memory.
auto FarByte(std::uint8_t* ds, UiHost const* host, FarPointer at) -> std::uint8_t& {
  auto const linear{ static_cast<std::uint32_t>(at.segment) * 16 + at.offset };
  auto const base{ static_cast<std::uint32_t>(host->ds_segment) * 16 };
  if (linear >= base && linear - base < 65536) return ds[linear - base];
  return host->memory[linear];
}

// The byte at the pointer, stepping the pointer past it.
auto FarNext(std::uint8_t* ds, UiHost const* host, FarPointer& at) -> std::uint8_t& {
  auto const old{ at };
  at.offset++;
  return FarByte(ds, host, old);
}

// The next byte of the picture stream (read before the stream pointer steps).
auto StreamNext(std::uint8_t* ds, UiHost const* host) -> std::uint8_t {
  return FarNext(ds, host, Data(ds).stream);
}

void BlitStream(std::uint8_t* ds, UiHost const* host, std::uint16_t x, std::uint16_t y) {
  auto const& stream{ Data(ds).stream };
  UiCall(ds, host, UiService::BLIT, x, y, 1, 32, 0, stream.segment, stream.offset);
}

// A cache of 0x780-byte image slots kept as a most-recently-used list of nodes; the head is a near pointer to a node.
struct Cache {
  std::array<CacheNode, 10>& nodes;
  std::uint16_t&             head;

  [[nodiscard]] auto Base(DataSegment const& data) const -> std::uint16_t {
    return Near(data, nodes);
  }
  // The index of the node a near pointer addresses.
  [[nodiscard]] auto Index(DataSegment const& data, std::uint16_t node) const -> std::int16_t {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(node - Base(data))) >> 1;
  }
  [[nodiscard]] auto NodeAt(DataSegment const& data, std::uint16_t index) const -> std::uint16_t {
    return static_cast<std::uint16_t>(Base(data) + 2 * index);
  }
  // The offset of the node's image slot in the cache's segment.
  [[nodiscard]] auto SlotOffset(DataSegment const& data, std::uint16_t node) const -> std::uint16_t {
    return static_cast<std::uint16_t>(Index(data, node) * IMAGE_BYTES);
  }
};

auto PortraitCache(DataSegment& data) -> Cache {
  return { data.portrait_nodes, data.portrait_head };
}

auto FaceCache(DataSegment& data) -> Cache {
  return { data.face_nodes, data.face_head };
}

// Takes a node out of the list (the head stays where it is).
void Unlink(DataSegment& data, Cache const& cache, std::uint16_t node) {
  if (node == cache.head) return;
  auto const    id{ static_cast<std::uint16_t>(cache.Index(data, node)) };
  std::uint16_t current{ cache.head };
  std::uint16_t slot;
  while ((slot = Record<CacheNode>(data, current).next) != id) {
    if (slot == END_OF_LIST) return;
    current = cache.NodeAt(data, slot);
  }
  Record<CacheNode>(data, current).next = Record<CacheNode>(data, cache.NodeAt(data, slot)).next;
}

// Makes a node the head of the list.
void Promote(DataSegment& data, Cache const& cache, std::uint16_t node) {
  if (node != cache.head) {
    Record<CacheNode>(data, node).next = static_cast<std::uint8_t>(cache.Index(data, cache.head));
    cache.head                         = node;
  }
}

// Finds the node holding `key`, or the last node in use order, and moves it to the head. Returns the node and whether
// it already held the key; a reused node takes the key.
auto Lookup(DataSegment& data, Cache const& cache, std::uint8_t key) -> std::pair<std::uint16_t, bool> {
  auto node{ cache.head };
  for (;;) {
    auto const held{ Record<CacheNode>(data, node).key };
    if (held == END_OF_LIST) break;
    if (held == key) {
      Unlink(data, cache, node);
      Promote(data, cache, node);
      return { node, true };
    }
    auto const next{ Record<CacheNode>(data, node).next };
    if (next == END_OF_LIST) break;
    node = cache.NodeAt(data, next);
  }
  Unlink(data, cache, node);
  Promote(data, cache, node);
  Record<CacheNode>(data, node).key = key;
  return { node, false };
}

} // namespace

// Reads a w x h picture of three planes from the file to the end of the image buffer and blits it.
void opeu_image_read_draw(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, int16_t handle) {
  auto const segment{ Data(ds).image_segment };
  auto const size{ Multiply(Multiply(h, w), 3) };
  if (size > IMAGE_END) return;
  ReadFar(ds, host, handle, segment, IMAGE_END - size, size);
  UiCall(ds, host, UiService::BLIT, x, y, w, h, mode, segment, IMAGE_END - size);
}

// Unpacks count bytes: a 0 or 0xff byte is followed by its repeat count.
void opeu_rle_expand(uint8_t ds[65536], const UiHost* host, uint16_t count, FarPointer src, FarPointer dst) {
  std::uint8_t  c{ 0 };
  std::uint16_t run{ 0 };
  while (count--) {
    if (run == 0) {
      c = FarNext(ds, host, src);
      if (c == 0 || c == 0xff) run = FarNext(ds, host, src);
    } else
      run--;
    FarNext(ds, host, dst) = c;
  }
}

// Draws the w x h mask at the end of the image buffer; mode 0 inverts the mask first and draws it in mode 0x407.
void opeu_image_draw_masked(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode) {
  auto const segment{ Data(ds).image_segment };
  auto const count{ Multiply(h, w) };
  if (static_cast<std::uint16_t>(count * 2) > IMAGE_END) return;
  FarPointer at{ static_cast<std::uint16_t>(IMAGE_END - count), segment };
  if (mode == 0) {
    for (auto left{ count }; left--; at.offset++) FarByte(ds, host, at) ^= 0xff;
    mode = 0x407;
  }
  UiCall(ds, host, UiService::DRAW, x, y, w, h, mode, segment, IMAGE_END - count);
}

// The decoder's reader for a picture file: refills the stream buffer from the picture file when it runs dry.
uint8_t opeu_stream_file_byte(uint8_t ds[65536], const UiHost* host) {
  auto& data{ Data(ds) };
  if (data.stream_count == 0) {
    data.stream.segment = data.image_segment;
    data.stream.offset  = STREAM_BUFFER;
    data.stream_count   = ReadFar(ds, host, data.picture_handle, data.image_segment, STREAM_BUFFER, STREAM_CHUNK);
  }
  data.stream_count--;
  return StreamNext(ds, host);
}

// Seeks the picture file to a 512-byte block.
void opeu_stream_seek_block(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t block) {
  (void)x;
  (void)y;
  auto& data{ Data(ds) };
  if (Seek(ds, host, data.picture_handle, static_cast<std::int32_t>(block) << 9, 0) != SEEK_FAILED) data.stream_count = 0;
}

void opeu_picture_file(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block) {
  Data(ds).picture_handle = handle;
  opeu_stream_seek_block(ds, host, x, y, block);
  UiCall(ds, host, UiService::DECODE, x, y, FILE_READER, host->code_segment);
}

void opeu_picture_file_masked(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block) {
  Data(ds).picture_handle = handle;
  opeu_stream_seek_block(ds, host, x, y, block);
  UiCall(ds, host, UiService::DECODE_MASKED, x, y, FILE_READER, host->code_segment);
}

// The decoder's reader for a picture in memory.
uint8_t opeu_stream_memory_byte(uint8_t ds[65536], const UiHost* host) {
  return StreamNext(ds, host);
}

void opeu_picture_memory(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t segment, uint16_t offset) {
  SetStream(Data(ds), segment, offset);
  UiCall(ds, host, UiService::DECODE, x, y, MEMORY_READER, host->code_segment);
}

void opeu_picture_memory_masked(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t segment, uint16_t offset) {
  SetStream(Data(ds), segment, offset);
  UiCall(ds, host, UiService::DECODE_MASKED, x, y, MEMORY_READER, host->code_segment);
}

void opeu_stream_seek(uint8_t ds[65536], const UiHost* host, int32_t position, int16_t handle) {
  if (Seek(ds, host, handle, position, 0) != SEEK_FAILED) Data(ds).stream_count = 0;
}

void opeu_picture_file_at(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, int16_t handle, int32_t position) {
  opeu_stream_seek(ds, host, position, handle);
  UiCall(ds, host, UiService::DECODE_MASKED, x, y, FILE_READER, host->code_segment);
}

// The frame of the context panel from the tile segment: left end, middle columns, right end; then the cursor inside.
void opeu_context_frame(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t width) {
  auto& data{ Data(ds) };
  SetStream(data, host->tiles_segment, 0);
  BlitStream(ds, host, x, y);
  SetStream(data, host->tiles_segment, 0x60);
  std::uint16_t i;
  for (i = x + 1; i < static_cast<std::uint16_t>(x + width - 1); i++) BlitStream(ds, host, i, y);
  SetStream(data, host->tiles_segment, 0xc0);
  BlitStream(ds, host, i, y);
  UiCall(ds, host, UiService::POSITION, x + 1, y + 8);
}

void opeu_portrait_unlink(uint8_t ds[65536], uint16_t node) {
  auto& data{ Data(ds) };
  Unlink(data, PortraitCache(data), node);
}

void opeu_portrait_promote(uint8_t ds[65536], uint16_t node) {
  auto& data{ Data(ds) };
  Promote(data, PortraitCache(data), node);
}

// The offset of the general's portrait in the portrait segment, read from the portrait file when not cached.
uint16_t opeu_portrait_load(uint8_t ds[65536], const UiHost* host, uint8_t key) {
  auto&      data{ Data(ds) };
  auto const cache{ PortraitCache(data) };
  auto const [node, cached]{ Lookup(data, cache, key) };
  auto const result{ cache.SlotOffset(data, node) };
  if (!cached) {
    Seek(ds, host, data.portrait_handle, static_cast<std::int32_t>(static_cast<std::uint32_t>(key) * IMAGE_BYTES), 0);
    ReadFar(ds, host, data.portrait_handle, data.portrait_segment, result, IMAGE_BYTES);
  }
  return result;
}

// Blits the general's portrait; the cache lookup runs before the portrait segment is read.
void opeu_context_portrait(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t person) {
  auto const offset{ opeu_portrait_load(ds, host, Record<Person>(Data(ds), person).portrait) };
  UiCall(ds, host, UiService::BLIT, x, y, 8, 80, 0, Data(ds).portrait_segment, offset);
}

void opeu_face_unlink(uint8_t ds[65536], uint16_t node) {
  auto& data{ Data(ds) };
  Unlink(data, FaceCache(data), node);
}

void opeu_face_promote(uint8_t ds[65536], uint16_t node) {
  auto& data{ Data(ds) };
  Promote(data, FaceCache(data), node);
}

// The offset of a face in the face segment; a face not cached is read packed from the face file into the image
// buffer and unpacked into its slot.
int16_t opeu_face_load(uint8_t ds[65536], const UiHost* host, uint8_t key) {
  auto&      data{ Data(ds) };
  auto const cache{ FaceCache(data) };
  auto const [node, cached]{ Lookup(data, cache, key) };
  if (cached) return static_cast<std::int16_t>(cache.SlotOffset(data, node));
  Seek(ds, host, data.face_handle, static_cast<std::int32_t>(static_cast<std::uint32_t>(data.face_offsets[key]) << 2), 0);
  ReadFar(ds, host, data.face_handle, data.image_segment, FACE_BUFFER, IMAGE_BYTES);
  SetStream(data, data.image_segment, FACE_BUFFER);
  auto const result{ cache.SlotOffset(data, node) };
  opeu_rle_expand(ds, host, IMAGE_BYTES, data.stream, FarPointer{ result, data.face_segment });
  return static_cast<std::int16_t>(result);
}

void opeu_far_copy(uint8_t ds[65536], const UiHost* host, uint16_t count, FarPointer src, FarPointer dst) {
  for (; count != 0; count--) {
    auto const value{ FarNext(ds, host, src) };
    FarNext(ds, host, dst) = value;
  }
}

// Draws a face masked: copied from its slot to the image buffer, then drawn from there.
void opeu_face_draw(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t mode, uint8_t key) {
  auto&            data{ Data(ds) };
  auto const       segment{ data.face_segment };
  FarPointer const source{ static_cast<std::uint16_t>(opeu_face_load(ds, host, key)), segment };
  SetStream(data, data.image_segment, FACE_BUFFER);
  opeu_far_copy(ds, host, IMAGE_BYTES, source, data.stream);
  opeu_image_draw_masked(ds, host, x, y, 24, 80, mode);
}

void opeu_ui_outline(uint8_t ds[65536], const UiHost* host, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  UiCall(ds, host, UiService::LINE, x1, y1, x2, y1, color);
  UiCall(ds, host, UiService::LINE, x2, y1, x2, y2, color);
  UiCall(ds, host, UiService::LINE, x1, y2, x2, y2, color);
  UiCall(ds, host, UiService::LINE, x1, y1, x1, y2, color);
}

// Fills the screen with a backdrop pattern: one 16-pixel band drawn, then copied down the screen.
void opeu_backdrop_fill(uint8_t ds[65536], const UiHost* host) {
  auto const pattern{ Near(Data(ds), Data(ds).backdrop_pattern) };
  for (std::uint16_t x = 0; x < 80; x += 2) UiCall(ds, host, UiService::DRAW_NEAR, x, 0, 2, 16, 1, pattern);
  for (std::uint16_t y = 16; y < 400; y += 16) UiCall(ds, host, UiService::COPY, 0, y - 16, 80, 16, 0, y);
}

void opeu_backdrop_fill_color(uint8_t ds[65536], const UiHost* host, uint8_t color) {
  auto const pattern{ Near(Data(ds), Data(ds).backdrop_color_pattern) };
  for (std::uint16_t x = 0; x < 80; x += 2) UiCall(ds, host, UiService::DRAW_NEAR, x, 0, 2, 16, color, pattern);
  for (std::uint16_t y = 16; y < 400; y += 16) UiCall(ds, host, UiService::COPY, 0, y - 16, 80, 16, 0, y);
}

// A blue screen with an outlined box for the text at (x, y); leaves the cursor there and returns the text.
uint16_t opeu_title_box(uint8_t ds[65536], const UiHost* host, uint16_t x, uint16_t y, uint16_t text) {
  auto const length{ static_cast<std::uint16_t>(opeu_string_length(ds, text) + 2) };
  opeu_backdrop_fill_color(ds, host, 1);
  UiCall(ds, host, UiService::BOX, x - 1, y - 4, length, 24, 1);
  opeu_ui_outline(ds, host, (x - 1) * 8, y - 4, (length + x) * 8 - 9, y + 19, 7);
  UiCall(ds, host, UiService::POSITION, x, y);
  return text;
}

// The base colors, then the upper eight for the hour: dawn, day, dusk or night.
void opeu_palette_for_hour(uint8_t ds[65536], const UiHost* host) {
  auto&         data{ Data(ds) };
  std::uint16_t palette{ 0 };
  UiCall(ds, host, UiService::SET_PALETTE, 0, 8, Near(data, data.palette_base));
  if (opeu_hour_is_dawn(data.hour)) palette = Near(data, data.palette_dawn);
  if (opeu_hour_is_day(data.hour)) palette = Near(data, data.palette_day);
  if (opeu_hour_is_dusk(data.hour)) palette = Near(data, data.palette_dusk);
  if (opeu_hour_is_night(data.hour)) palette = Near(data, data.palette_night);
  UiCall(ds, host, UiService::SET_PALETTE, 8, 8, palette);
}

void opeu_palette_daylight(uint8_t ds[65536], const UiHost* host) {
  auto& data{ Data(ds) };
  UiCall(ds, host, UiService::SET_PALETTE, 0, 8, Near(data, data.palette_base));
  UiCall(ds, host, UiService::SET_PALETTE, 8, 8, Near(data, data.palette_day));
}

void opeu_palette_title_screen(uint8_t ds[65536], const UiHost* host) {
  auto& data{ Data(ds) };
  UiCall(ds, host, UiService::SET_PALETTE, 0, 16, Near(data, data.palette_title));
}

void opeu_palette_map_screen(uint8_t ds[65536], const UiHost* host) {
  auto& data{ Data(ds) };
  UiCall(ds, host, UiService::SET_PALETTE, 0, 16, Near(data, data.palette_map));
}

// The UI primitives' services: file seek and read, the VGA library's drawing, the palette, the picture decoder with
// this module's readers, and the text cursor.
uint32_t svc_uiprim(uint8_t* ds, uint8_t* memory, uint16_t id, const uint16_t* args, uint16_t count) {
  (void)ds;
  (void)memory;
  switch (static_cast<UiService>(id)) {
  case UiService::SEEK: return (uint32_t)opeu_dos_seek((int16_t)Arg(args, count, 0), (int32_t)Arg32(args, count, 1), (int16_t)Arg(args, count, 3));
  case UiService::READ: return (uint16_t)opeu_dos_read_far((int16_t)Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3));
  case UiService::BLIT:
    opeu_blit_far(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    return 0;
  case UiService::DRAW:
    opeu_draw_far(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), port_far(Arg(args, count, 5), Arg(args, count, 6)));
    return 0;
  case UiService::DRAW_NEAR:     opeu_draw_near(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), PORT_DS(Arg(args, count, 5))); return 0;
  case UiService::LINE:          opeu_ui_line(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); return 0;
  case UiService::BOX:           opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); return 0;
  case UiService::COPY:          opeu_screen_copy(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5)); return 0;
  case UiService::SET_PALETTE:   opeu_set_palette((uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), PORT_DS(Arg(args, count, 2))); return 0;
  case UiService::DECODE:        opeu_picture_decode(Arg(args, count, 0), Arg(args, count, 1), port_picture_reader(Arg(args, count, 2), Arg(args, count, 3))); return 0;
  case UiService::DECODE_MASKED: opeu_picture_decode_masked(Arg(args, count, 0), Arg(args, count, 1), port_picture_reader(Arg(args, count, 2), Arg(args, count, 3))); return 0;
  case UiService::POSITION:      opeu_army_position(port_ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); return 0;
  default:                       port_unhandled("uiprim", id, count);
  }
  return 0;
}

auto port_routine_opeu_context_frame(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_context_frame(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2));
  return 0;
}

auto port_routine_opeu_context_portrait(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2));
  return 0;
}

auto port_routine_opeu_face_draw(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_face_draw(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), (uint8_t)Arg(args, count, 3));
  return 0;
}

auto port_routine_opeu_palette_for_hour(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_palette_for_hour(ds, (const UiHost*)port_uiprim_host());
  return 0;
}

auto port_routine_opeu_ui_outline(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
  return 0;
}
