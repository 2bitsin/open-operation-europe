// The game's battle-cell and sprite routines (SYSLIB drawing). Screen routines save and restore 40x26-pixel battle
// cells, set colour bit 3 over a rectangle, copy cell blocks and compose sprites through a memory buffer; buffer
// routines compose a cell in a 520-byte buffer (26 lines x 4 colour bits x 5 bytes, the bit blocks of a line at +0, +5,
// +10, +15, 8 pixels per byte, leftmost in bit 7). x is in 8-pixel columns unless noted, y in game lines. The pointer
// parameters are the game's (deferred to #17); every routine views them as spans of the sizes stated.
#pragma once

#include <cstdint>

inline constexpr auto SYSLIB_CELL_BYTES{ 520u }; // 26 lines x 20 bytes

// Copies a 520-byte cell buffer to the screen at byte column x, line y (5 bytes = 40 pixels wide, 26 lines).
auto opeu_vram_restore(std::uint16_t x, std::uint16_t y, std::uint8_t const* vram) -> void;
// The same at pixel column x * 8 + 4 (the cell straddles six bytes; the outer half-bytes of the screen are preserved).
auto opeu_vram_restore_odd(std::uint16_t x, std::uint16_t y, std::uint8_t const* vram) -> void;
// Copies the 40x26 screen cell at byte column x, line y into `vram`.
auto opeu_vram_save(std::uint16_t x, std::uint16_t y, std::uint8_t* vram) -> void;
// The same at pixel column x * 8 + 4.
auto opeu_vram_save_odd(std::uint16_t x, std::uint16_t y, std::uint8_t* vram) -> void;
// ORs 520 bytes of `src` into the cell buffer.
auto opeu_vram_draw(std::uint8_t* vram, std::uint8_t const* src) -> void;
// Near-source twin of opeu_vram_draw.
auto opeu_vram_draw_near(std::uint8_t* vram, std::uint8_t const* src) -> void;
// Draws a 6-line 8-pixel glyph at pixel (x, y) of the cell buffer in colour 2 (color == 0) or colour 1 (color != 0, low
// byte tested).
auto opeu_vram_glyph_color(std::uint8_t* vram, std::uint8_t const* glyph, std::uint16_t x, std::uint16_t y, std::uint16_t color) -> void;
// Sets colour bits 0-2 and clears bit 3 under a 6-line glyph (colour 7 over the low colours) at pixel (x, y) of the cell
// buffer.
auto opeu_vram_glyph(std::uint8_t* vram, std::uint8_t const* glyph, std::uint16_t x, std::uint16_t y) -> void;
// ANDs a 26x5-byte mask (130 bytes) into all four colour-bit blocks of the cell.
auto opeu_vram_load(std::uint8_t* vram, std::uint8_t const* mask) -> void;
// Sets colour bit 3 over a w-byte x h-line rectangle with the byte `color` (bits 0-2 untouched).
auto Koei3b77(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t color) -> void;
// Copies the 3-byte x 24-line screen block at (x1, y1) to (x2, y2).
auto koei_3bc5(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2) -> void;
// Saves the w-byte x h-line screen rectangle at (x, y) into `buffer` (colour-bit-major, 4 * w * h bytes), ANDs every
// colour bit with `and_mask` (w * h bytes), ORs `or_image` (3 bytes per byte column, or 4 when flags bit 0) into bits
// 0-2 (0-3), and writes the buffer back. In this argument order callers label and_mask "image" and or_image "mask".
auto opeu_sprite_compose(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* and_mask, std::uint8_t const* or_image, std::uint8_t* buffer, std::uint16_t flags)
    -> void;
// Pixel-positioned variant of opeu_sprite_compose: x and w in pixels. With flags bit 0 clear the buffer is composed and
// the screen left alone.
auto koei_3f26(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* and_mask, std::uint8_t const* or_image, std::uint8_t* buffer, std::uint16_t flags) -> void;
