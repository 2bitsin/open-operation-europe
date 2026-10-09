/* OCOMPOS (src/open/asm/ocompos.asm, OPEN.EXE _TEXT 1808..1E47) natively:
 * the buffer compositing and screen routines of the opening that MAIN's
 * library does not have (or has in a different form), over the planar VRAM
 * model of vgalib.h.
 *
 * Hand-written: these replace KOEI assembly; none is a game routine.
 *  - 1B56 opeu_open_bitmap_save: MAIN's 2ED8 without the PUSH DX / POP DX
 *    around the line address (no effect), but it writes through ES:DI, so
 *    the destination wraps inside its segment; written here on
 *    segment:offset for that reason.
 *  - 1BB9 _opeu_open_screen_copy is MAIN's 301F text (instruction-identical)
 *    and uses vgalib.c's opeu_screen_copy; it has no entry here.
 *  - 1C2F far copy, 1C74 OR compose, 1C94 AND mask, 1E2F dissolve: OPEN/END
 *    only.
 *  - 181D (the memory-source picture decoder) is not called by OPEN's C.
 *
 * VGA read map. 1B56 selects the plane to read with GC index 4 (Read Map
 * Select) and leaves it at 3. The dissolve (1E2F) reads the screen bytes it
 * keeps (`mov bh,[es:di]`) without selecting a plane, so it reads plane 3
 * for all four plane passes: where the mask bit is clear, planes 0-2 receive
 * plane 3's bit (the pixel turns 0 or 15) until a later step reveals it.
 * Within OPEN only 1B56 and the picture decoder (3096 copy, also ending at
 * plane 3) write GC index 4, and 1B56 runs before the first dissolve, so the
 * read map is always 3 there; it is tracked here (read_map) rather than
 * assumed.
 */
#include "port.hpp"
#include "vgalib.hpp"
#include "open-services.hpp"

static int read_map;            /* GC index 4; BIOS mode 12h sets 0 */

/* Byte at segment:offset of the data arena (16-bit offset arithmetic is the
 * caller's: pass the wrapped offset). */
static uint8_t *far_byte(uint16_t segment, uint16_t offset)
{
    return port_far(segment, offset);
}

static uint16_t line_address(uint16_t x, uint16_t y)
{
    return (uint16_t)((uint16_t)(y + VGA_TOP) * VGA_PITCH + x);
}

void opeu_open_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                           uint16_t segment, uint16_t offset)
{
    uint16_t si = line_address(x, y), di = offset, rows = h;
    do {
        uint16_t columns = w;
        do {
            int p;
            for (p = 0; p < 4; p++) {
                *far_byte(segment, di) = vga_plane[p][si];
                di = (uint16_t)(di + 1);
            }
            read_map = 3;
            si = (uint16_t)(si + 1);
        } while (--columns);
        si = (uint16_t)(si - w + 0x50);
    } while (--rows);
}

void opeu_open_far_copy(uint16_t source_segment, uint16_t source_offset,
                        uint16_t segment, uint16_t offset, uint16_t count)
{
    uint16_t words = (uint16_t)(count >> 1), si = source_offset, di = offset;
    while (words--) {           /* REP MOVSW: CX = 0 copies nothing */
        *far_byte(segment, di) = *far_byte(source_segment, si);
        *far_byte(segment, (uint16_t)(di + 1)) = *far_byte(source_segment, (uint16_t)(si + 1));
        si = (uint16_t)(si + 2);
        di = (uint16_t)(di + 2);
    }
}

void opeu_open_buf_or(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h,
                      uint16_t source_segment, uint16_t source_offset)
{
    uint16_t si = source_offset, di = offset, rows = h;
    do {                        /* 1C54: CX = rows, inner CX = 2 * columns (LOOP) */
        uint16_t words = (uint16_t)(w << 1);
        do {
            *far_byte(segment, di) |= *far_byte(source_segment, si);
            *far_byte(segment, (uint16_t)(di + 1)) |= *far_byte(source_segment, (uint16_t)(si + 1));
            di = (uint16_t)(di + 2);
            si = (uint16_t)(si + 2);
        } while (--words);
    } while (--rows);
}

void opeu_open_buf_and_mask(uint16_t segment, uint16_t offset, uint16_t w, uint16_t h,
                            uint16_t mask_segment, uint16_t mask_offset)
{
    uint16_t si = mask_offset, di = offset, rows = h;
    do {
        uint16_t pairs = (uint16_t)(w >> 1);
        do {
            uint8_t m0 = *far_byte(mask_segment, si);
            uint8_t m1 = *far_byte(mask_segment, (uint16_t)(si + 1));
            int k;
            for (k = 0; k < 8; k++) {
                *far_byte(segment, di) &= (k & 1) ? m1 : m0;
                di = (uint16_t)(di + 1);
            }
            si = (uint16_t)(si + 2);
        } while (--pairs);
    } while (--rows);
}

void opeu_open_dissolve_blit(uint16_t x, uint16_t y, uint16_t segment, uint16_t offset,
                             uint16_t pattern)
{
    uint16_t di = line_address(x, y), si = offset, dx = pattern;
    int row;
    vga_dirty = 1;
    for (row = 0; row < 0x120; row++) {
        uint16_t row_si = si, plane_si = si, d = di;
        int p;
        for (p = 0; p < 4; p++) {
            uint16_t s = plane_si;
            int c;
            d = di;
            for (c = 0; c < 0x13; c++) {
                uint16_t ax = (uint16_t)(*far_byte(segment, s) << 8 | *far_byte(segment, (uint16_t)(s + 4)));
                uint16_t bx = (uint16_t)(vga_plane[read_map][d] << 8 | vga_plane[read_map][(uint16_t)(d + 1)]);
                ax = (uint16_t)((ax & dx) | (bx & (uint16_t)~dx));
                vga_plane[p][d] = (uint8_t)(ax >> 8);
                vga_plane[p][(uint16_t)(d + 1)] = (uint8_t)ax;
                d = (uint16_t)(d + 2);
                s = (uint16_t)(s + 8);
            }
            plane_si = (uint16_t)(plane_si + 1);
        }
        dx = (uint16_t)(dx << 1 | dx >> 15);            /* ROL DX,1 */
        di = (uint16_t)(d + 0x2a);                      /* plane 3 pass left DI at the row end */
        si = (uint16_t)(row_si + 0x98);
    }
}

void koei_09b7(uint16_t a, uint16_t b)
{
    (void)a;
    (void)b;
}
