/* OPEN 032a:000e..01f1 (image 0x32ae..0x3492): EMS page mapping for the
 * sprite buffer, the one-buffer subset of MAIN's EMSMAP (06ac),
 * from which the C text below is copied.
 *
 * OPEN keeps only the sprite buffer in expanded memory: handle DS:1c08
 * (4 pages), flag DS:0180, segment variable DS:1c2c. DS:1bf8 holds the four
 * mappable physical pages (segment, page number) forming the page frame.
 * 000e maps `count` logical pages of a handle into consecutive physical
 * pages starting at `first`; 0048 maps the sprite buffer; 005c releases it
 * and reports success; 008a allocates `pages` pages if the EMM has that many
 * free; 00d0 allocates the sprite buffer and points its segment into the
 * page frame; 00f8 detects a LIM EMM whose page frame (at or above A000) is
 * four consecutive 16 KiB pages and copies their entries to DS:1bf8. The EMM
 * routines are the OPEN copy (frame 031c) of MAIN's hand-written segment 069d
 * (assembly).
 *
 * Every EMM call is a service (EmsEventId), as in MAIN's EMSMAP. Pointer
 * arguments are DS offsets of the original stack slots, derived from the
 * entry SP passed as `sp`.
 */
#include "recovered.hpp"

/* MAIN's MEMORY routine, linked into OPEN unchanged. */
void opeu_copy_bytes(uint8_t ds[65536], uint16_t source, uint16_t destination, uint16_t size);

namespace opeu::intro::detail::oemsmap {
namespace {
using enum EmsEventId;

/* A word in DGROUP at a near pointer; the second byte wraps within DS. */
auto WrappedWord(uint8_t const* ds, uint16_t at) -> uint16_t {
  return static_cast<uint16_t>(ds[at] | ds[static_cast<uint16_t>(at + 1)] << 8);
}

auto ems_service(uint8_t* ds, EmsEvent event, EmsEventId id, uint16_t count, uint16_t first, uint16_t second,
                 uint16_t third) -> uint16_t {
  uint16_t args[3];
  args[0] = first;
  args[1] = second;
  args[2] = third;
  return event(ds, id, args, count);
}

/* One EMM call with its argument words. */
template <class... Words>
auto Ems(uint8_t* ds, EmsEvent event, EmsEventId id, Words... words) -> uint16_t {
  uint16_t const padded[3]{ static_cast<uint16_t>(words)... };
  return ems_service(ds, event, id, sizeof...(Words), padded[0], padded[1], padded[2]);
}
}

/* 032a:000e. AX=handle, DX=count, BX=first physical page. */
void opeu_ems_map_pages(uint8_t* ds, EmsEvent event, int16_t handle, int16_t count, int16_t first) {
  int16_t i = 0;
  for (i = 0; i < count; i++) {
    Ems(ds, event, MAP, handle, Data(ds).ems_pages[first].number, i);
    first++;
  }
}

/* 032a:0048 */
void opeu_gfx_wait(uint8_t* ds, EmsEvent event) {
  if (Data(ds).sprite_mapped != 0) opeu_ems_map_pages(ds, event, static_cast<int16_t>(Data(ds).sprite_handle), 4, 0);
}

/* 032a:005c. 1 unless releasing the mapped handle failed. */
auto opeu_memory_release(uint8_t* ds, EmsEvent event) -> int16_t {
  int16_t ok = 0;
  ok         = 1;
  if ((Data(ds).sprite_mapped != 0) && (Ems(ds, event, RELEASE, Data(ds).sprite_handle) != 0u)) ok = 0;
  return ok;
}

/* 032a:008a. BX=handle variable, AX=pages. 1 if the pages were allocated.
 * Entered with SP=sp: BP=sp-2, total at BP-2, unallocated at BP-4. */
auto opeu_ems_reserve(uint8_t* ds, EmsEvent event, uint16_t sp, uint16_t handle, int16_t pages) -> int16_t {
  auto total       = static_cast<uint16_t>(sp - 4);
  auto unallocated = static_cast<uint16_t>(sp - 6);
  if (Ems(ds, event, FREE_PAGES, total, unallocated) == 0 && static_cast<int16_t>(WrappedWord(ds, unallocated)) >= pages
      && Ems(ds, event, ALLOCATE, pages, handle) == 0)
    return 1;
  return 0;
}

/* 032a:00d0. The near call to 008a pushes bp, the 2-byte frame and cs:ip. */
auto opeu_ems_alloc_sprite(uint8_t* ds, EmsEvent event, uint16_t sp) -> int16_t {
  int16_t ok = 0;
  ok         = opeu_ems_reserve(ds, event, static_cast<uint16_t>(sp - 8), offsetof(OpenDataSegment, sprite_handle), 4);
  if (ok != 0) Data(ds).buffer_f = Data(ds).ems_pages[0].segment;
  Data(ds).sprite_mapped = ok;
  return ok;
}

/* 032a:00f8. 1 if a usable EMM page frame was found and recorded. Entered
 * with SP=sp: BP=sp-2, pages at BP-100h, count BP-102h, frame BP-104h. */
auto opeu_ems_detect(uint8_t* ds, EmsEvent event, uint16_t sp) -> int16_t {
  auto    pages = static_cast<uint16_t>(sp - 0x102);
  auto    count = static_cast<uint16_t>(sp - 0x104);
  auto    frame = static_cast<uint16_t>(sp - 0x106);
  int16_t n     = 0;
  if (Ems(ds, event, PRESENT) == 0u) goto fail;
  if (Ems(ds, event, STATUS) != 0u) goto fail;
  if (Ems(ds, event, FRAME, frame) != 0u) goto fail;
  if (Ems(ds, event, PAGE_COUNT, count) != 0u) goto fail;
  if (static_cast<int16_t>(WrappedWord(ds, count)) > 64) goto fail;
  if (Ems(ds, event, PAGE_ARRAY, pages) != 0u) goto fail;
  if (WrappedWord(ds, frame) < 0xa000) goto fail;
  for (n = 0; n < static_cast<int16_t>(WrappedWord(ds, count)) - 1; n++) {
    if (WrappedWord(ds, static_cast<uint16_t>(pages + (4 * n))) == WrappedWord(ds, frame)) {
      if (static_cast<int16_t>(WrappedWord(ds, count)) - n < 4) goto fail;
      opeu_copy_bytes(ds, static_cast<uint16_t>(pages + (4 * n)), offsetof(OpenDataSegment, ems_pages), 16);
      for (n = 0; n < 3; n++)
        if (static_cast<uint16_t>(Data(ds).ems_pages[n].segment - Data(ds).ems_pages[n + 1].segment) != 0xfc00)
          goto fail;
      return 1;
    }
  }
fail:
  return 0;
}

}
