// UI primitives: pictures streamed from a file or from memory into the decoder, the portrait and face caches, the
// context frame, outlines and backdrops, the title box and the palettes for the time of day.
#include "core/uiprim.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/host.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "vgalib.hpp"

#include <cstdint>
#include <utility>

using namespace opeu::game;

namespace {

// Where the picture decoder's bytes come from: the open picture file, or a picture in memory.
enum class StreamSource : std::uint8_t {
  FILE,
  MEMORY,
};

// The decoder's byte reader of a source, bound to this game and host.
auto ReaderOf(Game& game, UiHost const* host, StreamSource source) -> PictureByteReader {
  if (source == StreamSource::FILE) return [&game, host] { return opeu_stream_file_byte(game, host); };
  return [&game, host] { return opeu_stream_memory_byte(game, host); };
}

// The image buffer segment: pictures are read and decoded below this offset, faces unpacked at FACE_BUFFER.
constexpr std::uint16_t IMAGE_END{ 0xc800 };
constexpr std::uint16_t FACE_BUFFER{ 0xc080 };
constexpr std::uint16_t STREAM_BUFFER{ 0xa000 }; // where a file picture is buffered
constexpr std::uint16_t STREAM_CHUNK{ 0x2800 };

constexpr std::uint16_t IMAGE_BYTES{ 0x780 }; // a portrait or face: 24 x 80 pixels
constexpr std::uint8_t  END_OF_LIST{ 0xff };
constexpr std::int32_t  SEEK_FAILED{ -1 };

auto Seek(Game& game, UiHost const* host, std::int16_t handle, std::int32_t position, std::uint16_t origin) -> std::int32_t {
  return opeu_dos_seek(handle, position, static_cast<std::int16_t>(origin));
}

auto ReadFar(Game& game, UiHost const* host, std::int16_t handle, std::uint16_t segment, std::uint16_t offset, std::uint16_t count) -> std::uint16_t {
  return static_cast<std::uint16_t>(opeu_dos_read_far(handle, segment, offset, count));
}

auto Multiply(std::uint16_t a, std::uint16_t b) -> std::uint16_t {
  return static_cast<std::uint16_t>(static_cast<std::uint32_t>(a) * b);
}

// The byte a far pointer addresses: in the data segment when it aliases it, otherwise in the host's flat memory.
auto FarByte(Game& game, UiHost const* host, FarPointer at) -> std::uint8_t& {
  auto const linear{ static_cast<std::uint32_t>(at.segment) * 16 + at.offset };
  auto const base{ static_cast<std::uint32_t>(host->ds_segment) * 16 };
  if (linear >= base && linear - base < 65536) return Byte(game, linear - base);
  return host->memory[linear];
}

// The byte at the pointer, stepping the pointer past it.
auto FarNext(Game& game, UiHost const* host, FarPointer& at) -> std::uint8_t& {
  auto const old{ at };
  at.offset++;
  return FarByte(game, host, old);
}

// The next byte of the picture stream (read before the stream pointer steps).
auto StreamNext(Game& game, UiHost const* host) -> std::uint8_t {
  return FarNext(game, host, Data(game).stream);
}

void BlitStream(Game& game, UiHost const* host, std::uint16_t x, std::uint16_t y) {
  auto const& stream{ Data(game).stream };
  opeu_blit_far(x, y, 1, 32, 0, port_far(stream.segment, stream.offset));
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
void opeu_image_read_draw(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, int16_t handle) {
  auto const segment{ Data(game).image_segment };
  auto const size{ Multiply(Multiply(h, w), 3) };
  if (size > IMAGE_END) return;
  ReadFar(game, host, handle, segment, IMAGE_END - size, size);
  opeu_blit_far(x, y, w, h, mode, port_far(segment, IMAGE_END - size));
}

// Unpacks count bytes: a 0 or 0xff byte is followed by its repeat count.
void opeu_rle_expand(Game& game, const UiHost* host, uint16_t count, FarPointer src, FarPointer dst) {
  std::uint8_t  c{ 0 };
  std::uint16_t run{ 0 };
  while (count--) {
    if (run == 0) {
      c = FarNext(game, host, src);
      if (c == 0 || c == 0xff) run = FarNext(game, host, src);
    } else
      run--;
    FarNext(game, host, dst) = c;
  }
}

// Draws the w x h mask at the end of the image buffer; mode 0 inverts the mask first and draws it in mode 0x407.
void opeu_image_draw_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode) {
  auto const segment{ Data(game).image_segment };
  auto const count{ Multiply(h, w) };
  if (static_cast<std::uint16_t>(count * 2) > IMAGE_END) return;
  FarPointer at{ static_cast<std::uint16_t>(IMAGE_END - count), segment };
  if (mode == 0) {
    for (auto left{ count }; left--; at.offset++) FarByte(game, host, at) ^= 0xff;
    mode = 0x407;
  }
  opeu_draw_far(x, y, w, h, mode, port_far(segment, IMAGE_END - count));
}

// The decoder's reader for a picture file: refills the stream buffer from the picture file when it runs dry.
uint8_t opeu_stream_file_byte(Game& game, const UiHost* host) {
  auto& data{ Data(game) };
  if (data.stream_count == 0) {
    data.stream.segment = data.image_segment;
    data.stream.offset  = STREAM_BUFFER;
    data.stream_count   = ReadFar(game, host, data.picture_handle, data.image_segment, STREAM_BUFFER, STREAM_CHUNK);
  }
  data.stream_count--;
  return StreamNext(game, host);
}

// Seeks the picture file to a 512-byte block.
void opeu_stream_seek_block(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t block) {
  (void)x;
  (void)y;
  auto& data{ Data(game) };
  if (Seek(game, host, data.picture_handle, static_cast<std::int32_t>(block) << 9, 0) != SEEK_FAILED) data.stream_count = 0;
}

void opeu_picture_file(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block) {
  Data(game).picture_handle = handle;
  opeu_stream_seek_block(game, host, x, y, block);
  opeu_picture_decode(x, y, ReaderOf(game, host, StreamSource::FILE));
}

void opeu_picture_file_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, uint16_t block) {
  Data(game).picture_handle = handle;
  opeu_stream_seek_block(game, host, x, y, block);
  opeu_picture_decode_masked(x, y, ReaderOf(game, host, StreamSource::FILE));
}

// The decoder's reader for a picture in memory.
uint8_t opeu_stream_memory_byte(Game& game, const UiHost* host) {
  return StreamNext(game, host);
}

void opeu_picture_memory(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t segment, uint16_t offset) {
  SetStream(Data(game), segment, offset);
  opeu_picture_decode(x, y, ReaderOf(game, host, StreamSource::MEMORY));
}

void opeu_picture_memory_masked(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t segment, uint16_t offset) {
  SetStream(Data(game), segment, offset);
  opeu_picture_decode_masked(x, y, ReaderOf(game, host, StreamSource::MEMORY));
}

void opeu_stream_seek(Game& game, const UiHost* host, int32_t position, int16_t handle) {
  if (Seek(game, host, handle, position, 0) != SEEK_FAILED) Data(game).stream_count = 0;
}

void opeu_picture_file_at(Game& game, const UiHost* host, uint16_t x, uint16_t y, int16_t handle, int32_t position) {
  opeu_stream_seek(game, host, position, handle);
  opeu_picture_decode_masked(x, y, ReaderOf(game, host, StreamSource::FILE));
}

// The frame of the context panel from the tile segment: left end, middle columns, right end; then the cursor inside.
void opeu_context_frame(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t width) {
  auto& data{ Data(game) };
  SetStream(data, host->tiles_segment, 0);
  BlitStream(game, host, x, y);
  SetStream(data, host->tiles_segment, 0x60);
  std::uint16_t i;
  for (i = x + 1; i < static_cast<std::uint16_t>(x + width - 1); i++) BlitStream(game, host, i, y);
  SetStream(data, host->tiles_segment, 0xc0);
  BlitStream(game, host, i, y);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 8));
}

void opeu_portrait_unlink(Game& game, uint16_t node) {
  auto& data{ Data(game) };
  Unlink(data, PortraitCache(data), node);
}

void opeu_portrait_promote(Game& game, uint16_t node) {
  auto& data{ Data(game) };
  Promote(data, PortraitCache(data), node);
}

// The offset of the general's portrait in the portrait segment, read from the portrait file when not cached.
uint16_t opeu_portrait_load(Game& game, const UiHost* host, uint8_t key) {
  auto&      data{ Data(game) };
  auto const cache{ PortraitCache(data) };
  auto const [node, cached]{ Lookup(data, cache, key) };
  auto const result{ cache.SlotOffset(data, node) };
  if (!cached) {
    Seek(game, host, data.portrait_handle, static_cast<std::int32_t>(static_cast<std::uint32_t>(key) * IMAGE_BYTES), 0);
    ReadFar(game, host, data.portrait_handle, data.portrait_segment, result, IMAGE_BYTES);
  }
  return result;
}

// Blits the general's portrait; the cache lookup runs before the portrait segment is read.
void opeu_context_portrait(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t person) {
  auto const offset{ opeu_portrait_load(game, host, Record<Person>(Data(game), person).portrait) };
  opeu_blit_far(x, y, 8, 80, 0, port_far(Data(game).portrait_segment, offset));
}

void opeu_face_unlink(Game& game, uint16_t node) {
  auto& data{ Data(game) };
  Unlink(data, FaceCache(data), node);
}

void opeu_face_promote(Game& game, uint16_t node) {
  auto& data{ Data(game) };
  Promote(data, FaceCache(data), node);
}

// The offset of a face in the face segment; a face not cached is read packed from the face file into the image
// buffer and unpacked into its slot.
int16_t opeu_face_load(Game& game, const UiHost* host, uint8_t key) {
  auto&      data{ Data(game) };
  auto const cache{ FaceCache(data) };
  auto const [node, cached]{ Lookup(data, cache, key) };
  if (cached) return static_cast<std::int16_t>(cache.SlotOffset(data, node));
  Seek(game, host, data.face_handle, static_cast<std::int32_t>(static_cast<std::uint32_t>(data.face_offsets[key]) << 2), 0);
  ReadFar(game, host, data.face_handle, data.image_segment, FACE_BUFFER, IMAGE_BYTES);
  SetStream(data, data.image_segment, FACE_BUFFER);
  auto const result{ cache.SlotOffset(data, node) };
  opeu_rle_expand(game, host, IMAGE_BYTES, data.stream, FarPointer{ result, data.face_segment });
  return static_cast<std::int16_t>(result);
}

void opeu_far_copy(Game& game, const UiHost* host, uint16_t count, FarPointer src, FarPointer dst) {
  for (; count != 0; count--) {
    auto const value{ FarNext(game, host, src) };
    FarNext(game, host, dst) = value;
  }
}

// Draws a face masked: copied from its slot to the image buffer, then drawn from there.
void opeu_face_draw(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t mode, uint8_t key) {
  auto&            data{ Data(game) };
  auto const       segment{ data.face_segment };
  FarPointer const source{ static_cast<std::uint16_t>(opeu_face_load(game, host, key)), segment };
  SetStream(data, data.image_segment, FACE_BUFFER);
  opeu_far_copy(game, host, IMAGE_BYTES, source, data.stream);
  opeu_image_draw_masked(game, host, x, y, 24, 80, mode);
}

void opeu_ui_outline(Game& game, const UiHost* host, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  opeu_ui_line(x1, y1, x2, y1, color);
  opeu_ui_line(x2, y1, x2, y2, color);
  opeu_ui_line(x1, y2, x2, y2, color);
  opeu_ui_line(x1, y1, x1, y2, color);
}

// Fills the screen with a backdrop pattern: one 16-pixel band drawn, then copied down the screen.
void opeu_backdrop_fill(Game& game, const UiHost* host) {
  auto const pattern{ Near(Data(game), Data(game).backdrop_pattern) };
  for (std::uint16_t x = 0; x < 80; x += 2) opeu_draw_near(x, 0, 2, 16, 1, game.State().Bytes(pattern).data());
  for (std::uint16_t y = 16; y < 400; y += 16) opeu_screen_copy(0, y - 16, 80, 16, 0, y);
}

void opeu_backdrop_fill_color(Game& game, const UiHost* host, uint8_t color) {
  auto const pattern{ Near(Data(game), Data(game).backdrop_color_pattern) };
  for (std::uint16_t x = 0; x < 80; x += 2) opeu_draw_near(x, 0, 2, 16, color, game.State().Bytes(pattern).data());
  for (std::uint16_t y = 16; y < 400; y += 16) opeu_screen_copy(0, y - 16, 80, 16, 0, y);
}

// A blue screen with an outlined box for the text at (x, y); leaves the cursor there and returns the text.
uint16_t opeu_title_box(Game& game, const UiHost* host, uint16_t x, uint16_t y, uint16_t text) {
  auto const length{ static_cast<std::uint16_t>(opeu_string_length(game, text) + 2) };
  opeu_backdrop_fill_color(game, host, 1);
  opeu_ui_box(x - 1, y - 4, length, 24, 1);
  opeu_ui_outline(game, host, (x - 1) * 8, y - 4, (length + x) * 8 - 9, y + 19, 7);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
  return text;
}

// The base colors, then the upper eight for the hour: dawn, day, dusk or night.
void opeu_palette_for_hour(Game& game, const UiHost* host) {
  auto&         data{ Data(game) };
  std::uint16_t palette{ 0 };
  opeu_set_palette(0, 8, data.palette_base.data());
  if (opeu_hour_is_dawn(data.hour)) palette = Near(data, data.palette_dawn);
  if (opeu_hour_is_day(data.hour)) palette = Near(data, data.palette_day);
  if (opeu_hour_is_dusk(data.hour)) palette = Near(data, data.palette_dusk);
  if (opeu_hour_is_night(data.hour)) palette = Near(data, data.palette_night);
  opeu_set_palette(8, 8, game.State().Bytes(palette).data());
}

void opeu_palette_daylight(Game& game, const UiHost* host) {
  auto& data{ Data(game) };
  opeu_set_palette(0, 8, data.palette_base.data());
  opeu_set_palette(8, 8, data.palette_day.data());
}

void opeu_palette_title_screen(Game& game, const UiHost* host) {
  auto& data{ Data(game) };
  opeu_set_palette(0, 16, data.palette_title.data());
}

void opeu_palette_map_screen(Game& game, const UiHost* host) {
  auto& data{ Data(game) };
  opeu_set_palette(0, 16, data.palette_map.data());
}
