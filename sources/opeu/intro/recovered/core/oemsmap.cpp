/* OPEN 032a:000e..01f1 (image 0x32ae..0x3492): EMS page mapping for the
 * sprite buffer, the one-buffer subset of MAIN's EMSMAP (06ac,
 * src/main/core/emsmap.c), from which the C text below is copied.
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
 * (src/main/asm/emsdrv.asm).
 *
 * Host builds keep DGROUP in ds[] and report every EMM call to a scripted
 * service exactly as MAIN's EMSMAP host branch does: 0 present(), 1 status(),
 * 2 page_count(&count), 3 free_pages(&total, &unallocated), 4 frame(&segment),
 * 5 allocate(pages, &handle), 6 page_array(pages), 7 map(handle, physical,
 * logical), 8 release(handle). Pointer arguments are DS offsets of the
 * original stack slots, derived from the entry SP passed as `sp`.
 */
#include "types.hpp"
typedef uint16_t (*EmsEvent)(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
void opeu_copy_bytes(uint8_t ds[65536], uint16_t source, uint16_t destination, uint16_t size);
void opeu_ems_map_pages(uint8_t ds[65536], EmsEvent event, int16_t handle, int16_t count, int16_t first);
void opeu_gfx_wait(uint8_t ds[65536], EmsEvent event);
int16_t opeu_memory_release(uint8_t ds[65536], EmsEvent event);
int16_t opeu_ems_reserve(uint8_t ds[65536], EmsEvent event, uint16_t sp, uint16_t handle, int16_t pages);
int16_t opeu_ems_alloc_sprite(uint8_t ds[65536], EmsEvent event, uint16_t sp);
int16_t opeu_ems_detect(uint8_t ds[65536], EmsEvent event, uint16_t sp);
static uint16_t ems_word(const uint8_t* ds, uint16_t at) {
  return (uint16_t)(ds[at] | ds[(uint16_t)(at + 1)] << 8);
}
static void ems_set_word(uint8_t* ds, uint16_t at, uint16_t value) {
  ds[at] = (uint8_t)value;
  ds[(uint16_t)(at + 1)] = (uint8_t)(value >> 8);
}
static uint16_t ems_service(uint8_t* ds, EmsEvent event, uint16_t id, uint16_t count, uint16_t first, uint16_t second,
                            uint16_t third) {
  uint16_t args[3];
  args[0] = first;
  args[1] = second;
  args[2] = third;
  return event(ds, id, args, count);
}
static int16_t ems_result(uint8_t* ds, uint16_t segment, uint16_t mapped, uint16_t value, int16_t flag) {
  if (flag) ems_set_word(ds, segment, value);
  ems_set_word(ds, mapped, (uint16_t)flag);
  return flag;
}
#define STATE uint8_t ds[65536], EmsEvent event,
#define VOID_STATE uint8_t ds[65536], EmsEvent event
#define SP_STATE uint8_t ds[65536], EmsEvent event, uint16_t sp
#define PASS ds, event,
#define PASS_VOID ds, event
/* A near call from an entry with SP=sp: push bp, the 2-byte frame, push cs/ip. */
#define PASS_SP(n) ds, event, (uint16_t)(sp - (n)),
#define HANDLE_PTR uint16_t
#define PAGE_NUMBER(i) ems_word(ds, (uint16_t)(0x1bfa + 4 * (i)))
#define PAGE_SEGMENT(i) ems_word(ds, (uint16_t)(0x1bf8 + 4 * (i)))
#define SPRITE_MAPPED ems_word(ds, 0x0180)
#define SPRITE_HANDLE ems_word(ds, 0x1c08)
#define SPRITE_HANDLE_AT 0x1c08
#define SP_ARG uint8_t ds[65536], EmsEvent event, uint16_t sp,
#define SPRITE_RESULT(v) return ems_result(ds, 0x1c2c, 0x0180, PAGE_SEGMENT(0), v)
/* 008a entered with SP=sp: BP=sp-2, total at BP-2, unallocated at BP-4. */
#define RESERVE_LOCALS uint16_t total = (uint16_t)(sp - 4), unallocated = (uint16_t)(sp - 6)
#define AT_TOTAL total
#define AT_UNALLOCATED unallocated
#define UNALLOCATED ((int16_t)ems_word(ds, unallocated))
/* 00f8 entered with SP=sp: BP=sp-2, pages at BP-100h, count BP-102h, frame BP-104h. */
#define DETECT_LOCALS \
  uint16_t pages = (uint16_t)(sp - 0x102), count = (uint16_t)(sp - 0x104), frame = (uint16_t)(sp - 0x106)
#define AT_PAGES pages
#define AT_PAGE(i) (uint16_t)(pages + 4 * (i))
#define AT_COUNT count
#define AT_FRAME frame
#define COUNT ((int16_t)ems_word(ds, count))
#define FRAME ems_word(ds, frame)
#define LOCAL_PAGE(i) ems_word(ds, (uint16_t)(pages + 4 * (i)))
#define EMS_PAGES 0x1bf8
#define EMS_PRESENT() ems_service(ds, event, 0, 0, 0, 0, 0)
#define EMS_STATUS() ems_service(ds, event, 1, 0, 0, 0, 0)
#define EMS_PAGE_COUNT(p) ems_service(ds, event, 2, 1, p, 0, 0)
#define EMS_FREE_PAGES(t, u) ems_service(ds, event, 3, 2, t, u, 0)
#define EMS_FRAME(p) ems_service(ds, event, 4, 1, p, 0, 0)
#define EMS_ALLOCATE(n, h) ems_service(ds, event, 5, 2, (uint16_t)(n), h, 0)
#define EMS_PAGE_ARRAY(p) ems_service(ds, event, 6, 1, p, 0, 0)
#define EMS_MAP(h, p, l) ems_service(ds, event, 7, 3, (uint16_t)(h), p, (uint16_t)(l))
#define EMS_RELEASE(h) ems_service(ds, event, 8, 1, (uint16_t)(h), 0, 0)
#define COPY(s, d, n) opeu_copy_bytes(ds, s, d, n)

/* 032a:000e. AX=handle, DX=count, BX=first physical page. */
void opeu_ems_map_pages(STATE int16_t handle, int16_t count, int16_t first) {
  int16_t i;
  for (i = 0; i < count; i++) {
    EMS_MAP(handle, PAGE_NUMBER(first), i);
    first++;
  }
}

/* 032a:0048 */
void opeu_gfx_wait(VOID_STATE) {
  if (SPRITE_MAPPED) opeu_ems_map_pages(PASS SPRITE_HANDLE, 4, 0);
}

/* 032a:005c. 1 unless releasing the mapped handle failed. */
int16_t opeu_memory_release(VOID_STATE) {
  int16_t ok;
  ok = 1;
  if (SPRITE_MAPPED && EMS_RELEASE(SPRITE_HANDLE)) ok = 0;
  return ok;
}

/* 032a:008a. BX=handle variable, AX=pages. 1 if the pages were allocated. */
int16_t opeu_ems_reserve(SP_ARG HANDLE_PTR handle, int16_t pages) {
  RESERVE_LOCALS;
  if (EMS_FREE_PAGES(AT_TOTAL, AT_UNALLOCATED) == 0 && UNALLOCATED >= pages && EMS_ALLOCATE(pages, handle) == 0)
    return 1;
  return 0;
}

/* 032a:00d0 */
int16_t opeu_ems_alloc_sprite(SP_STATE) {
  int16_t ok;
  ok = opeu_ems_reserve(PASS_SP(8) SPRITE_HANDLE_AT, 4);
  SPRITE_RESULT(ok);
}

/* 032a:00f8. 1 if a usable EMM page frame was found and recorded. */
int16_t opeu_ems_detect(SP_STATE) {
  DETECT_LOCALS;
  int16_t n;
  if (!EMS_PRESENT()) goto fail;
  if (EMS_STATUS()) goto fail;
  if (EMS_FRAME(AT_FRAME)) goto fail;
  if (EMS_PAGE_COUNT(AT_COUNT)) goto fail;
  if (COUNT > 64) goto fail;
  if (EMS_PAGE_ARRAY(AT_PAGES)) goto fail;
  if (FRAME < 0xa000) goto fail;
  for (n = 0; n < COUNT - 1; n++) {
    if (LOCAL_PAGE(n) == FRAME) {
      if (COUNT - n < 4) goto fail;
      COPY(AT_PAGE(n), EMS_PAGES, 16);
      for (n = 0; n < 3; n++)
        if ((uint16_t)(PAGE_SEGMENT(n) - PAGE_SEGMENT(n + 1)) != 0xfc00) goto fail;
      return 1;
    }
  }
fail:
  return 0;
}
