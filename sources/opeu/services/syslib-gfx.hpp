/* Native C reimplementation of the drawing routines of KOEI's SYSLIB
 * (MAIN.EXE src/main/asm/syslib.asm, _TEXT 34B8..3FCD) over the planar VRAM
 * model of vgalib.h (four 64 KiB planes, screen byte (y + 40) * 80 + x).
 *
 * Two groups:
 *  - VRAM routines (save/restore of 40x26-pixel battle cells, plane-3 fill,
 *    VRAM-to-VRAM cell copy, sprite composition through a system-memory
 *    buffer);
 *  - "vram" buffer routines that never touch the VGA: they compose a cell in
 *    a 520-byte DS buffer (26 rows x 4 planes x 5 bytes, row-major, the plane
 *    blocks of a row at +0, +5, +10, +15) that the save/restore routines
 *    exchange with the screen.
 *
 * Far (segment, offset) pairs and near DS pointers become host pointers.
 * Coordinates keep the original units (x in bytes unless noted).
 */
#pragma once

#include <stdint.h>

#define SYSLIB_CELL_BYTES 520u      /* 26 rows * 20 bytes */

/* 34B8: copy a 520-byte cell buffer to the screen at byte column x, line y
 * (5 bytes = 40 pixels wide, 26 lines). */
void opeu_vram_restore(uint16_t x, uint16_t y, const uint8_t* vram);
/* 354F: the same at pixel column x * 8 + 4 (the cell straddles six bytes;
 * the outer half-bytes of the screen are preserved). */
void opeu_vram_restore_odd(uint16_t x, uint16_t y, const uint8_t* vram);
/* 37E4: copy the 40x26 screen cell at byte column x, line y into `vram`. */
void opeu_vram_save(uint16_t x, uint16_t y, uint8_t* vram);
/* 3882: the same at pixel column x * 8 + 4. */
void opeu_vram_save_odd(uint16_t x, uint16_t y, uint8_t* vram);
/* 3A04: OR 520 bytes of `src` into the cell buffer. */
void opeu_vram_draw(uint8_t* vram, const uint8_t* src);
/* 3A2A: near-source twin of opeu_vram_draw. */
void opeu_vram_draw_near(uint8_t* vram, const uint8_t* src);
/* 3A4D: draw a 6-row 8-pixel glyph at pixel (x, y) of the cell buffer in
 * colour 2 (color == 0) or colour 1 (color != 0, low byte tested). */
void opeu_vram_glyph_color(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y, uint16_t color);
/* 3AC3: OR planes 0-2 and clear plane 3 under a 6-row glyph (colour 7 over
 * the low colours) at pixel (x, y) of the cell buffer. */
void opeu_vram_glyph(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y);
/* 3B1E: AND a 26x5-byte mask (130 bytes) into all four planes of the cell. */
void opeu_vram_load(uint8_t* vram, const uint8_t* mask);
/* 3B77: fill plane 3 of a w-byte x h-line rectangle with the byte `color`
 * (other planes untouched). */
void koei_3b77(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
/* 3BC5: copy the 3-byte x 24-line screen block at (x1, y1) to (x2, y2). */
void koei_3bc5(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
/* 3E0A: save the w-byte x h-line screen rectangle at (x, y) into `buffer`
 * (plane-major, 4 * w * h bytes), AND every plane with `and_mask` (w * h
 * bytes), OR `or_image` (3 bytes per byte column, or 4 when flags bit 0) into
 * planes 0-2 (0-3), and write the buffer back.  Argument order is the
 * original's: callers label and_mask "image" and or_image "mask". */
void opeu_sprite_compose(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask,
                         const uint8_t* or_image, uint8_t* buffer, uint16_t flags);
/* 3F26: pixel-positioned variant of opeu_sprite_compose: x and w in pixels.
 * The original never returns when flags bit 0 is clear (see syslib_gfx.c). */
void koei_3f26(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask, const uint8_t* or_image,
               uint8_t* buffer, uint16_t flags);
