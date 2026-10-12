// The game's battle-cell and sprite routines (SYSLIB drawing). Screen routines save and restore 40x26-pixel battle
// cells, set colour bit 3 over a rectangle, copy cell blocks and compose sprites through a memory buffer; buffer
// routines compose a cell in a 520-byte buffer (26 lines x 4 colour bits x 5 bytes, the bit blocks of a line at +0, +5,
// +10, +15, 8 pixels per byte, leftmost in bit 7). x is in 8-pixel columns unless noted, y in game lines.
#pragma once

#include <stdint.h>

inline constexpr auto SYSLIB_CELL_BYTES{ 520u }; /* 26 rows * 20 bytes */

/* copy a 520-byte cell buffer to the screen at byte column x, line y
 * (5 bytes = 40 pixels wide, 26 lines). */
void opeu_vram_restore(uint16_t x, uint16_t y, const uint8_t* vram);
/* the same at pixel column x * 8 + 4 (the cell straddles six bytes;
 * the outer half-bytes of the screen are preserved). */
void opeu_vram_restore_odd(uint16_t x, uint16_t y, const uint8_t* vram);
/* Copy the 40x26 screen cell at byte column x, line y into `vram`. */
void opeu_vram_save(uint16_t x, uint16_t y, uint8_t* vram);
/* the same at pixel column x * 8 + 4. */
void opeu_vram_save_odd(uint16_t x, uint16_t y, uint8_t* vram);
/* OR 520 bytes of `src` into the cell buffer. */
void opeu_vram_draw(uint8_t* vram, const uint8_t* src);
/* near-source twin of opeu_vram_draw. */
void opeu_vram_draw_near(uint8_t* vram, const uint8_t* src);
/* draw a 6-row 8-pixel glyph at pixel (x, y) of the cell buffer in
 * colour 2 (color == 0) or colour 1 (color != 0, low byte tested). */
void opeu_vram_glyph_color(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y, uint16_t color);
/* Set colour bits 0-2 and clear bit 3 under a 6-row glyph (colour 7 over
 * the low colours) at pixel (x, y) of the cell buffer. */
void opeu_vram_glyph(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y);
/* AND a 26x5-byte mask (130 bytes) into all four colour-bit blocks of the cell. */
void opeu_vram_load(uint8_t* vram, const uint8_t* mask);
/* Set colour bit 3 over a w-byte x h-line rectangle with the byte `color`
 * (bits 0-2 untouched). */
void koei_3b77(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
/* copy the 3-byte x 24-line screen block at (x1, y1) to (x2, y2). */
void koei_3bc5(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
/* save the w-byte x h-line screen rectangle at (x, y) into `buffer`
 * (colour-bit-major, 4 * w * h bytes), AND every colour bit with `and_mask` (w * h
 * bytes), OR `or_image` (3 bytes per byte column, or 4 when flags bit 0) into
 * bits 0-2 (0-3), and write the buffer back.  In this argument order callers label and_mask "image" and
 * or_image "mask". */
void opeu_sprite_compose(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask,
                         const uint8_t* or_image, uint8_t* buffer, uint16_t flags);
/* pixel-positioned variant of opeu_sprite_compose: x and w in pixels. With flags bit 0
 * clear the buffer is composed and the screen left alone. */
void koei_3f26(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask, const uint8_t* or_image,
               uint8_t* buffer, uint16_t flags);
