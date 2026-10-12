// Expanded memory for the large graphics buffers: when conventional memory runs short, the sprite font (4 pages), the
// numeric-dialog image (2), the map tiles (2) and the frame image (1, at physical page 3) live in EMS pages mapped
// into the 64 KiB page frame. ems_pages holds the frame's four physical pages as (segment, page number) pairs.
#include "core/emsmap.hpp"
#include "common/events.hpp"
#include "core/memory.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The EMM services, called through the event. Pointer arguments are DS offsets of words the EMM writes.
enum Service : std::uint16_t {
  PRESENT    = 0, // () -> nonzero when an EMM is installed
  STATUS     = 1, // () -> 0 when it works
  PAGE_COUNT = 2, // (&count) -> 0 on success
  FREE_PAGES = 3, // (&total, &unallocated) -> 0 on success
  FRAME      = 4, // (&segment) -> 0 on success
  ALLOCATE   = 5, // (pages, &handle) -> 0 on success
  PAGE_ARRAY = 6, // (pages) -> 0 on success: (segment, page number) of each mappable page
  MAP        = 7, // (handle, physical page, logical page) -> 0 on success
  RELEASE    = 8, // (handle) -> 0 on success
};

constexpr std::uint16_t EMS_PAGES{ offsetof(DataSegment, ems_pages) };
constexpr std::uint16_t MAX_PAGES{ 64 }; // the page array the detection reads has room for 64 entries
constexpr std::uint16_t FRAME_LOWEST{ 0xa000 }; // a page frame below this segment is not usable
constexpr std::uint16_t PAGE_PARAGRAPHS{ 0x400 }; // one 16 KiB page

auto PageSegment(DataSegment const& data, int i) -> std::uint16_t {
  return data.ems_pages[2 * i];
}
auto PageNumber(DataSegment const& data, int i) -> std::uint16_t {
  return data.ems_pages[2 * i + 1];
}

// A buffer's allocation result: on success its segment variable points at the page it is mapped to; the mapped flag
// records the result either way.
auto Result(std::uint16_t& segment, std::uint16_t& mapped, std::uint16_t page_segment, int16_t ok) -> int16_t {
  if (ok) segment = page_segment;
  mapped = static_cast<std::uint16_t>(ok);
  return ok;
}

// The SP an allocation passes to opeu_ems_reserve: its own frame and return address below its entry SP.
constexpr auto ReserveSp(std::uint16_t sp) -> std::uint16_t {
  return static_cast<std::uint16_t>(sp - 8);
}
} // namespace

// Maps `count` logical pages of the handle into consecutive physical pages starting at `first`.
void opeu_ems_map_pages(uint8_t ds[65536], HostEvent event, int16_t handle, int16_t count, int16_t first) {
  auto const& data{ Data(ds) };
  for (int16_t i{ 0 }; i < count; ++i, ++first) Call(ds, event, MAP, handle, PageNumber(data, first), i);
}

// Maps the map tiles in before the map view draws.
void opeu_view_prepare(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (data.tile_mapped) opeu_ems_map_pages(ds, event, static_cast<int16_t>(data.tile_ems_handle), 2, 0);
}

// Maps the numeric-dialog image in.
void opeu_ems_map_dialog(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (data.numdlg_image_mapped) opeu_ems_map_pages(ds, event, static_cast<int16_t>(data.numdlg_ems_handle), 2, 0);
}

// Maps the sprite font in.
void opeu_gfx_wait(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (data.sprite_mapped) opeu_ems_map_pages(ds, event, static_cast<int16_t>(data.sprite_ems_handle), 4, 0);
}

// Maps the frame image in at physical page 3.
void opeu_display_prepare(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  if (data.frame_mapped) opeu_ems_map_pages(ds, event, static_cast<int16_t>(data.frame_ems_handle), 2, 3);
}

// Releases every mapped buffer's handle; 1 unless a release failed.
int16_t opeu_memory_release(uint8_t ds[65536], HostEvent event) {
  auto const& data{ Data(ds) };
  int16_t     ok{ 1 };
  if (data.frame_mapped && Call(ds, event, RELEASE, data.frame_ems_handle)) ok = 0;
  if (data.sprite_mapped && Call(ds, event, RELEASE, data.sprite_ems_handle)) ok = 0;
  if (data.numdlg_image_mapped && Call(ds, event, RELEASE, data.numdlg_ems_handle)) ok = 0;
  if (data.tile_mapped && Call(ds, event, RELEASE, data.tile_ems_handle)) ok = 0;
  return ok;
}

// Allocates `pages` pages into the handle variable at `handle` if the EMM has that many unallocated; 1 on success.
// The EMM writes the free-page counts into two words of the caller's frame below `sp`.
int16_t opeu_ems_reserve(uint8_t ds[65536], HostEvent event, uint16_t sp, uint16_t handle, int16_t pages) {
  auto const total{ static_cast<std::uint16_t>(sp - 4) };
  auto const unallocated{ static_cast<std::uint16_t>(sp - 6) };
  return Call(ds, event, FREE_PAGES, total, unallocated) == 0 && static_cast<int16_t>(Word(ds, unallocated)) >= pages
                 && Call(ds, event, ALLOCATE, pages, handle) == 0
             ? 1
             : 0;
}

int16_t opeu_ems_alloc_sprite(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const ok{ opeu_ems_reserve(ds, event, ReserveSp(sp), offsetof(DataSegment, sprite_ems_handle), 4) };
  return Result(data.sprite_segment, data.sprite_mapped, PageSegment(data, 0), ok);
}

int16_t opeu_ems_alloc_numdlg(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const ok{ opeu_ems_reserve(ds, event, ReserveSp(sp), offsetof(DataSegment, numdlg_ems_handle), 2) };
  return Result(data.numdlg_image_segment, data.numdlg_image_mapped, PageSegment(data, 0), ok);
}

int16_t opeu_ems_alloc_tile(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const ok{ opeu_ems_reserve(ds, event, ReserveSp(sp), offsetof(DataSegment, tile_ems_handle), 2) };
  return Result(data.tile_segment, data.tile_mapped, PageSegment(data, 0), ok);
}

int16_t opeu_ems_alloc_frame(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const ok{ opeu_ems_reserve(ds, event, ReserveSp(sp), offsetof(DataSegment, frame_ems_handle), 1) };
  return Result(data.frame_segment, data.frame_mapped, PageSegment(data, 3), ok);
}

// Detects an EMM whose page frame (at or above A000) is four consecutive 16 KiB pages and records those pages in
// ems_pages; 1 on success. The EMM writes its answers into the frame below `sp`.
int16_t opeu_ems_detect(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto const pages{ static_cast<std::uint16_t>(sp - 0x102) };
  auto const count_at{ static_cast<std::uint16_t>(sp - 0x104) };
  auto const frame_at{ static_cast<std::uint16_t>(sp - 0x106) };
  auto const count{ [&] { return static_cast<int16_t>(Word(ds, count_at)); } };
  if (!Call(ds, event, PRESENT) || Call(ds, event, STATUS) || Call(ds, event, FRAME, frame_at)
      || Call(ds, event, PAGE_COUNT, count_at) || count() > MAX_PAGES || Call(ds, event, PAGE_ARRAY, pages)
      || Word(ds, frame_at) < FRAME_LOWEST)
    return 0;
  for (int16_t n{ 0 }; n < count() - 1; ++n) {
    auto const entry{ static_cast<std::uint16_t>(pages + 4 * n) };
    if (Word(ds, entry) != Word(ds, frame_at)) continue;
    if (count() - n < 4) return 0;
    opeu_copy_bytes(ds, entry, EMS_PAGES, 16);
    auto const& data{ Data(ds) };
    for (int i{ 0 }; i < 3; ++i)
      if (static_cast<std::uint16_t>(PageSegment(data, i) - PageSegment(data, i + 1))
          != static_cast<std::uint16_t>(-PAGE_PARAGRAPHS))
        return 0;
    return 1;
  }
  return 0;
}

uint16_t svc_emsmap(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  (void)ds;
  switch (id) {
  case PRESENT:    return static_cast<uint16_t>(opeu_ems_present());
  case STATUS:     return static_cast<uint16_t>(opeu_ems_status());
  case PAGE_COUNT: return static_cast<uint16_t>(opeu_ems_page_count((int16_t*)PORT_DS(Arg(args, count, 0))));
  case FREE_PAGES:
    return static_cast<uint16_t>(
        opeu_ems_free_pages((int16_t*)PORT_DS(Arg(args, count, 0)), (int16_t*)PORT_DS(Arg(args, count, 1))));
  case FRAME: return static_cast<uint16_t>(opeu_ems_frame((uint16_t*)PORT_DS(Arg(args, count, 0))));
  case ALLOCATE:
    return static_cast<uint16_t>(
        opeu_ems_allocate((int16_t)Arg(args, count, 0), (int16_t*)PORT_DS(Arg(args, count, 1))));
  case PAGE_ARRAY: return static_cast<uint16_t>(opeu_ems_page_array(PORT_DS(Arg(args, count, 0))));
  case MAP:
    return static_cast<uint16_t>(
        opeu_ems_map((int16_t)Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2)));
  case RELEASE: return static_cast<uint16_t>(opeu_ems_release((int16_t)Arg(args, count, 0)));
  default:      port_unhandled("emsmap", id, count);
  }
  return 0;
}

auto port_routine_opeu_gfx_wait(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_gfx_wait(ds, svc_emsmap);
  return 0;
}
