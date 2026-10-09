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
#include "types.h"
#if defined(_MSC_VER) && _MSC_VER < 1000
typedef struct { uint16_t segment, number; } EmsPage;
extern EmsPage opeu_ems_pages[];
extern int16_t opeu_sprite_ems_handle;
extern int16_t opeu_sprite_mapped;
extern uint16_t opeu_sprite_segment;
extern int _cdecl opeu_ems_present(void);
extern int _cdecl opeu_ems_status(void);
extern int _cdecl opeu_ems_page_count(int16_t *count);
extern int _cdecl opeu_ems_free_pages(int16_t *total, int16_t *available);
extern int _cdecl opeu_ems_frame(uint16_t *segment);
extern int _cdecl opeu_ems_allocate(int16_t pages, int16_t *handle);
extern int _cdecl opeu_ems_page_array(EmsPage *pages);
extern int _cdecl opeu_ems_map(int16_t handle, uint16_t physical, int16_t logical);
extern int _cdecl opeu_ems_release(int16_t handle);
extern void opeu_copy_bytes(const void *source, void *destination, uint16_t size);
void opeu_ems_map_pages(int16_t handle, int16_t count, int16_t first);
void opeu_gfx_wait(void);
int16_t opeu_memory_release(void);
int16_t opeu_ems_reserve(int16_t *handle, int16_t pages);
int16_t opeu_ems_alloc_sprite(void);
int16_t opeu_ems_detect(void);
#pragma alloc_text(C032A_EMS,opeu_ems_map_pages,opeu_gfx_wait,opeu_memory_release,opeu_ems_reserve)
#pragma alloc_text(C032A_EMS,opeu_ems_alloc_sprite,opeu_ems_detect)
#define STATE
#define VOID_STATE void
#define SP_STATE void
#define PASS
#define PASS_VOID
#define PASS_SP(n)
#define HANDLE_PTR int16_t *
#define PAGE_NUMBER(i) opeu_ems_pages[i].number
#define PAGE_SEGMENT(i) opeu_ems_pages[i].segment
#define SPRITE_MAPPED opeu_sprite_mapped
#define SPRITE_HANDLE opeu_sprite_ems_handle
#define SPRITE_HANDLE_AT &opeu_sprite_ems_handle
#define SP_ARG
#define ALLOC_RESULT(segment, mapped, value, flag) if (flag) segment = value; return mapped = flag
#define SPRITE_RESULT(v) ALLOC_RESULT(opeu_sprite_segment, opeu_sprite_mapped, opeu_ems_pages[0].segment, v)
#define RESERVE_LOCALS int16_t total, unallocated
#define AT_TOTAL &total
#define AT_UNALLOCATED &unallocated
#define UNALLOCATED unallocated
#define DETECT_LOCALS EmsPage pages[64]; int16_t count; uint16_t frame
#define AT_PAGES pages
#define AT_PAGE(i) &pages[i]
#define AT_COUNT &count
#define AT_FRAME &frame
#define COUNT count
#define FRAME frame
#define LOCAL_PAGE(i) pages[i].segment
#define EMS_PAGES opeu_ems_pages
#define EMS_PRESENT() opeu_ems_present()
#define EMS_STATUS() opeu_ems_status()
#define EMS_PAGE_COUNT(p) opeu_ems_page_count(p)
#define EMS_FREE_PAGES(t,u) opeu_ems_free_pages(t,u)
#define EMS_FRAME(p) opeu_ems_frame(p)
#define EMS_ALLOCATE(n,h) opeu_ems_allocate(n,h)
#define EMS_PAGE_ARRAY(p) opeu_ems_page_array(p)
#define EMS_MAP(h,p,l) opeu_ems_map(h,p,l)
#define EMS_RELEASE(h) opeu_ems_release(h)
#define COPY(s,d,n) opeu_copy_bytes(s,d,n)
#else
typedef uint16_t (*EmsEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
void opeu_copy_bytes(uint8_t ds[65536], uint16_t source, uint16_t destination, uint16_t size);
void opeu_ems_map_pages(uint8_t ds[65536], EmsEvent event, int16_t handle, int16_t count, int16_t first);
void opeu_gfx_wait(uint8_t ds[65536], EmsEvent event);
int16_t opeu_memory_release(uint8_t ds[65536], EmsEvent event);
int16_t opeu_ems_reserve(uint8_t ds[65536], EmsEvent event, uint16_t sp, uint16_t handle, int16_t pages);
int16_t opeu_ems_alloc_sprite(uint8_t ds[65536], EmsEvent event, uint16_t sp);
int16_t opeu_ems_detect(uint8_t ds[65536], EmsEvent event, uint16_t sp);
static uint16_t ems_word(const uint8_t *ds, uint16_t at)
{
    return (uint16_t)(ds[at] | ds[(uint16_t)(at+1)] << 8);
}
static void ems_set_word(uint8_t *ds, uint16_t at, uint16_t value)
{
    ds[at] = (uint8_t)value;
    ds[(uint16_t)(at+1)] = (uint8_t)(value >> 8);
}
static uint16_t ems_service(uint8_t *ds, EmsEvent event, uint16_t id, uint16_t count,
                            uint16_t first, uint16_t second, uint16_t third)
{
    uint16_t args[3];
    args[0] = first;
    args[1] = second;
    args[2] = third;
    return event(ds, id, args, count);
}
static int16_t ems_result(uint8_t *ds, uint16_t segment, uint16_t mapped, uint16_t value, int16_t flag)
{
    if (flag)
        ems_set_word(ds, segment, value);
    ems_set_word(ds, mapped, (uint16_t)flag);
    return flag;
}
#define STATE uint8_t ds[65536], EmsEvent event,
#define VOID_STATE uint8_t ds[65536], EmsEvent event
#define SP_STATE uint8_t ds[65536], EmsEvent event, uint16_t sp
#define PASS ds, event,
#define PASS_VOID ds, event
/* A near call from an entry with SP=sp: push bp, the 2-byte frame, push cs/ip. */
#define PASS_SP(n) ds, event, (uint16_t)(sp-(n)),
#define HANDLE_PTR uint16_t
#define PAGE_NUMBER(i) ems_word(ds, (uint16_t)(0x1bfa + 4*(i)))
#define PAGE_SEGMENT(i) ems_word(ds, (uint16_t)(0x1bf8 + 4*(i)))
#define SPRITE_MAPPED ems_word(ds, 0x0180)
#define SPRITE_HANDLE ems_word(ds, 0x1c08)
#define SPRITE_HANDLE_AT 0x1c08
#define SP_ARG uint8_t ds[65536], EmsEvent event, uint16_t sp,
#define SPRITE_RESULT(v) return ems_result(ds, 0x1c2c, 0x0180, PAGE_SEGMENT(0), v)
/* 008a entered with SP=sp: BP=sp-2, total at BP-2, unallocated at BP-4. */
#define RESERVE_LOCALS uint16_t total = (uint16_t)(sp-4), unallocated = (uint16_t)(sp-6)
#define AT_TOTAL total
#define AT_UNALLOCATED unallocated
#define UNALLOCATED ((int16_t)ems_word(ds, unallocated))
/* 00f8 entered with SP=sp: BP=sp-2, pages at BP-100h, count BP-102h, frame BP-104h. */
#define DETECT_LOCALS uint16_t pages = (uint16_t)(sp-0x102), count = (uint16_t)(sp-0x104), \
    frame = (uint16_t)(sp-0x106)
#define AT_PAGES pages
#define AT_PAGE(i) (uint16_t)(pages+4*(i))
#define AT_COUNT count
#define AT_FRAME frame
#define COUNT ((int16_t)ems_word(ds, count))
#define FRAME ems_word(ds, frame)
#define LOCAL_PAGE(i) ems_word(ds, (uint16_t)(pages+4*(i)))
#define EMS_PAGES 0x1bf8
#define EMS_PRESENT() ems_service(ds, event, 0, 0, 0, 0, 0)
#define EMS_STATUS() ems_service(ds, event, 1, 0, 0, 0, 0)
#define EMS_PAGE_COUNT(p) ems_service(ds, event, 2, 1, p, 0, 0)
#define EMS_FREE_PAGES(t,u) ems_service(ds, event, 3, 2, t, u, 0)
#define EMS_FRAME(p) ems_service(ds, event, 4, 1, p, 0, 0)
#define EMS_ALLOCATE(n,h) ems_service(ds, event, 5, 2, (uint16_t)(n), h, 0)
#define EMS_PAGE_ARRAY(p) ems_service(ds, event, 6, 1, p, 0, 0)
#define EMS_MAP(h,p,l) ems_service(ds, event, 7, 3, (uint16_t)(h), p, (uint16_t)(l))
#define EMS_RELEASE(h) ems_service(ds, event, 8, 1, (uint16_t)(h), 0, 0)
#define COPY(s,d,n) opeu_copy_bytes(ds, s, d, n)
#endif

/* 032a:000e. AX=handle, DX=count, BX=first physical page. */
void opeu_ems_map_pages(STATE int16_t handle, int16_t count, int16_t first)
{
    int16_t i;
    for (i = 0; i < count; i++) {
        EMS_MAP(handle, PAGE_NUMBER(first), i);
        first++;
    }
}

/* 032a:0048 */
void opeu_gfx_wait(VOID_STATE)
{
    if (SPRITE_MAPPED)
        opeu_ems_map_pages(PASS SPRITE_HANDLE, 4, 0);
}

/* 032a:005c. 1 unless releasing the mapped handle failed. */
int16_t opeu_memory_release(VOID_STATE)
{
    int16_t ok;
    ok = 1;
    if (SPRITE_MAPPED && EMS_RELEASE(SPRITE_HANDLE))
        ok = 0;
    return ok;
}

#if defined(_MSC_VER) && _MSC_VER < 1000
/* Unused declarations: C 6.0a orders the operands of `unallocated >= pages`
 * by identifier number (mod 17). Four to six more identifiers before 008a
 * give OPEN's `cmp si,[bp-4]; jg` instead of MAIN's `cmp [bp-4],si; jl`
 * (the same expression); 0..3 and 7..8 extra give MAIN's order. They emit
 * no EXTDEF. */
extern int16_t opeu_ems_unused1, opeu_ems_unused2, opeu_ems_unused3, opeu_ems_unused4, opeu_ems_unused5;
#endif
/* 032a:008a. BX=handle variable, AX=pages. 1 if the pages were allocated. */
int16_t opeu_ems_reserve(SP_ARG HANDLE_PTR handle, int16_t pages)
{
    RESERVE_LOCALS;
    if (EMS_FREE_PAGES(AT_TOTAL, AT_UNALLOCATED) == 0 && UNALLOCATED >= pages &&
            EMS_ALLOCATE(pages, handle) == 0)
        return 1;
    return 0;
}

/* 032a:00d0 */
int16_t opeu_ems_alloc_sprite(SP_STATE)
{
    int16_t ok;
    ok = opeu_ems_reserve(PASS_SP(8) SPRITE_HANDLE_AT, 4);
    SPRITE_RESULT(ok);
}

/* 032a:00f8. 1 if a usable EMM page frame was found and recorded. */
int16_t opeu_ems_detect(SP_STATE)
{
    DETECT_LOCALS;
    int16_t n;
    if (!EMS_PRESENT())
        goto fail;
    if (EMS_STATUS())
        goto fail;
    if (EMS_FRAME(AT_FRAME))
        goto fail;
    if (EMS_PAGE_COUNT(AT_COUNT))
        goto fail;
    if (COUNT > 64)
        goto fail;
    if (EMS_PAGE_ARRAY(AT_PAGES))
        goto fail;
    if (FRAME < 0xa000)
        goto fail;
    for (n = 0; n < COUNT - 1; n++) {
        if (LOCAL_PAGE(n) == FRAME) {
            if (COUNT - n < 4)
                goto fail;
            COPY(AT_PAGE(n), EMS_PAGES, 16);
            for (n = 0; n < 3; n++)
                if ((uint16_t)(PAGE_SEGMENT(n) - PAGE_SEGMENT(n+1)) != 0xfc00)
                    goto fail;
            return 1;
        }
    }
fail:
    return 0;
}
