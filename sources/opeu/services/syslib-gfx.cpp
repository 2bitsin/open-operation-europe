// The game's battle-cell and sprite routines (SYSLIB drawing) on the canvas. The cell buffers and sprite images stay in
// the game's form, one byte per colour bit for 8 pixels; the screen side reads and writes those bits of the pixels.
#include "syslib-gfx.hpp"

#include "canvas.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>

using namespace opeu::services;

namespace {

// The canvas position of column x (8 pixels) on game line y; columns past 80 continue on the next line.
[[nodiscard]] auto ColumnAt(uint16_t x, std::int64_t y) noexcept -> Offset {
  return GameAt(std::int64_t{ x } * 8, y);
}

// A loop count of 0 runs 65536 times.
[[nodiscard]] auto LoopCount(uint16_t n) noexcept -> uint32_t {
  return n ? n : 0x10000u;
}

using Cell = std::span<uint8_t, SYSLIB_CELL_BYTES>;
constexpr auto CELL_LINES{ 26 };

// A cell line is 20 bytes: 5 columns of colour bit 0, then of bits 1, 2 and 3.
auto SaveCell(Offset at, Cell cell) noexcept -> void {
  for (auto const row : std::views::iota(0, CELL_LINES)) {
    auto const line{ cell.subspan(static_cast<std::size_t>(row) * 20, 20) };
    for (auto const column : std::views::iota(0, 5))
      for (auto const [plane, bits] : std::views::zip(std::views::iota(0), Screen().Column(at + row * 640 + column * 8))) line[static_cast<std::size_t>(plane * 5 + column)] = bits;
  }
}

auto RestoreCell(Offset at, std::span<uint8_t const, SYSLIB_CELL_BYTES> cell) noexcept -> void {
  for (auto const row : std::views::iota(0, CELL_LINES)) {
    auto const line{ cell.subspan(static_cast<std::size_t>(row) * 20, 20) };
    for (auto const column : std::views::iota(0, 5)) Screen().SetColumn(at + row * 640 + column * 8, { line[column], line[column + 5uz], line[column + 10uz], line[column + 15uz] });
  }
}

// The w-column x h-line rectangle from `at`, colour-bit-major (all of bit 0, then bit 1, ...); returns the bytes per
// colour bit.
auto SaveRect(Offset at, uint16_t w, uint16_t h, uint8_t* buffer) noexcept -> uint32_t {
  auto const      count{ std::size_t{ LoopCount(h) } * w };
  std::span const target{ buffer, 4 * count };
  for (auto const row : std::views::iota(0u, LoopCount(h)))
    for (auto const column : std::views::iota(0u, uint32_t{ w }))
      for (auto const [plane, bits] : std::views::zip(std::views::iota(0uz), Screen().Column(at + Offset{ row } * 640 + column * 8))) target[plane * count + row * w + column] = bits;
  return static_cast<uint32_t>(count);
}

auto RestoreRect(Offset at, uint16_t w, uint16_t h, uint8_t const* buffer) noexcept -> void {
  auto const      count{ std::size_t{ LoopCount(h) } * w };
  std::span const source{ buffer, 4 * count };
  for (auto const row : std::views::iota(0u, LoopCount(h)))
    for (auto const column : std::views::iota(0u, uint32_t{ w })) {
      auto const i{ std::size_t{ row } * w + column };
      Screen().SetColumn(at + Offset{ row } * 640 + column * 8, { source[i], source[count + i], source[2 * count + i], source[3 * count + i] });
    }
}

}

// ---- battle cells (26 lines x 5 columns) -------------------------------------------------------------------------

// The cell buffer: per line, 5 bytes of each colour bit 0..3.
void opeu_vram_restore(uint16_t x, uint16_t y, const uint8_t* vram) {
  RestoreCell(ColumnAt(x, y), std::span<uint8_t const, SYSLIB_CELL_BYTES>{ vram, SYSLIB_CELL_BYTES });
}

// The cell starts half a column right (pixel x * 8 + 4); the pixels around it keep their colours.
void opeu_vram_restore_odd(uint16_t x, uint16_t y, const uint8_t* vram) {
  RestoreCell(ColumnAt(x, y) + 4, std::span<uint8_t const, SYSLIB_CELL_BYTES>{ vram, SYSLIB_CELL_BYTES });
}

void opeu_vram_save(uint16_t x, uint16_t y, uint8_t* vram) {
  SaveCell(ColumnAt(x, y), Cell{ vram, SYSLIB_CELL_BYTES });
}

void opeu_vram_save_odd(uint16_t x, uint16_t y, uint8_t* vram) {
  SaveCell(ColumnAt(x, y) + 4, Cell{ vram, SYSLIB_CELL_BYTES });
}

// ---- cell-buffer composition (memory only) -------------------------------------------------------------------

// ORs a source cell into the cell buffer.
void opeu_vram_draw(uint8_t* vram, const uint8_t* src) {
  unsigned i;
  for (i = 0; i < SYSLIB_CELL_BYTES; i++) vram[i] |= src[i];
}

void opeu_vram_draw_near(uint8_t* vram, const uint8_t* src) {
  opeu_vram_draw(vram, src);
}

// A glyph byte shifted right by c pixels across a byte pair: lo = g >> c, hi = g << (8 - c).
static void GlyphPair(uint8_t g, unsigned c, uint8_t* lo, uint8_t* hi) {
  uint16_t ax = (uint16_t)(((unsigned)g >> c) | ((unsigned)g << (16 - c)));
  *lo         = (uint8_t)ax;
  *hi         = (uint8_t)(ax >> 8);
}

// OR / AND of a byte pair of the cell buffer. A glyph in column 4 spills its second byte into column 0 of the next
// colour bit's block (or of the next line); the game's cell glyphs rely on nothing else.
static void Or16(uint8_t* p, uint8_t lo, uint8_t hi) {
  p[0] |= lo;
  p[1] |= hi;
}
static void And16(uint8_t* p, uint8_t lo, uint8_t hi) {
  p[0] &= lo;
  p[1] &= hi;
}

// A 6-line glyph at pixel (x, y) of the cell buffer in colour 2 (low byte of color 0) or colour 1.
void opeu_vram_glyph_color(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y, uint16_t color) {
  uint8_t* di = vram + (uint16_t)((uint16_t)(y * 20u) + (x >> 3));
  unsigned c  = x & 7;
  int      row;
  for (row = 0; row < 6; row++, di += 20) {
    uint8_t lo, hi;
    GlyphPair(*glyph++, c, &lo, &hi);
    if ((uint8_t)color == 0) {
      And16(di, (uint8_t)~lo, (uint8_t)~hi);
      Or16(di + 5, lo, hi);
    } else {
      Or16(di, lo, hi);
      And16(di + 5, (uint8_t)~lo, (uint8_t)~hi);
    }
    And16(di + 10, (uint8_t)~lo, (uint8_t)~hi);
    And16(di + 15, (uint8_t)~lo, (uint8_t)~hi);
  }
}

// A 6-line glyph in colour 7 over colours 0-7: bits 0-2 set, bit 3 cleared.
void opeu_vram_glyph(uint8_t* vram, const uint8_t* glyph, uint16_t x, uint16_t y) {
  uint8_t* di = vram + (uint16_t)((uint16_t)(y * 20u) + (x >> 3));
  unsigned c  = x & 7;
  int      row;
  for (row = 0; row < 6; row++, di += 20) {
    uint8_t lo, hi;
    GlyphPair(*glyph++, c, &lo, &hi);
    Or16(di, lo, hi);
    Or16(di + 5, lo, hi);
    Or16(di + 10, lo, hi);
    And16(di + 15, (uint8_t)~lo, (uint8_t)~hi);
  }
}

// ANDs a 26x5-byte mask into the four colour-bit blocks of every line.
void opeu_vram_load(uint8_t* vram, const uint8_t* mask) {
  int row, i;
  for (row = 0; row < 26; row++, vram += 20, mask += 5)
    for (i = 0; i < 5; i++) {
      vram[i]      &= mask[i];
      vram[i + 5]  &= mask[i];
      vram[i + 10] &= mask[i];
      vram[i + 15] &= mask[i];
    }
}

// ---- screen fills and copies ------------------------------------------------------------------------------------

// Bit 3 of the colours of a w-column x h-line rectangle from the bits of the low byte of `color` (h = 0: 65536 lines);
// bits 0-2 keep. Unclipped.
void Koei3b77(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  for (auto const row : std::views::iota(0u, LoopCount(h)))
    for (auto const column : std::views::iota(int{ x }, x + w)) Screen().SetPlaneBits(ColumnAt(static_cast<uint16_t>(column), y) + Offset{ row } * 640, 3, static_cast<uint8_t>(color));
}

// Copies the 3-column x 24-line block at (x1, y1) to (x2, y2), line by line from the top; within a line the first two
// columns are read before either is written.
void koei_3bc5(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  auto&      screen{ Screen() };
  auto const from{ ColumnAt(x1, y1) };
  auto const to{ ColumnAt(x2, y2) };
  for (auto const line : std::views::iota(0, 24) | std::views::transform([](int row) { return Offset{ row } * 640; })) {
    screen.Move<16>(to + line, from + line);
    screen.Move<8>(to + line + 16, from + line + 16);
  }
}

// n = w * h in 16 bits; a product of 0 runs the AND and OR passes 65536 times.
void opeu_sprite_compose(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask, const uint8_t* or_image, uint8_t* buffer, uint16_t flags) {
  auto const at{ ColumnAt(x, y) };
  uint16_t   n16 = (uint16_t)((uint32_t)w * h);
  uint32_t   n   = LoopCount(n16), i;
  int        p;
  SaveRect(at, w, h, buffer);
  for (p = 0; p < 4; p++)
    for (i = 0; i < n; i++) buffer[p * (uint32_t)n16 + i] &= and_mask[i];
  for (i = 0; i < n; i++) {
    buffer[i]            |= *or_image++;
    buffer[i + n16]      |= *or_image++;
    buffer[i + 2u * n16] |= *or_image++;
    if (flags & 1) buffer[i + 3u * n16] |= *or_image++;
  }
  RestoreRect(at, w, h, buffer);
}

// ANDs one colour-bit block of the buffer (W bytes x h lines) with the mask (W - 1 bytes per line) shifted right by c
// pixels, ones shifted in; W = 1 runs the middle 65535 times.
static uint8_t* AndShifted(uint8_t* d, const uint8_t* m, unsigned c, uint16_t w, uint16_t h) {
  uint32_t rows = LoopCount(h), r;
  for (r = 0; r < rows; r++) {
    uint16_t dx  = (uint16_t)(w - 2);
    *d++        &= (uint8_t)((0xFF00u | *m) >> c);
    for (; dx; dx--, m++) *d++ &= (uint8_t)((((unsigned)m[0] << 8) | m[1]) >> c);
    if (c) *d &= (uint8_t)((((unsigned)m[0] << 8) | 0xFFu) >> c);
    d++;
    m++;
  }
  return d;
}

// ORs one colour bit of the interleaved image (stride 3 + flags) into the next block, shifted right by c pixels, zeros
// shifted in.
static uint8_t* OrShifted(uint8_t* d, const uint8_t* s, unsigned c, uint16_t w, uint16_t h, uint16_t stride) {
  uint32_t rows = LoopCount(h), r;
  for (r = 0; r < rows; r++) {
    uint16_t dx  = (uint16_t)(w - 2);
    *d++        |= (uint8_t)(*s >> c);
    for (; dx; dx--) {
      unsigned hi  = *s;
      s           += stride;
      *d++        |= (uint8_t)(((hi << 8) | *s) >> c);
    }
    if (c) *d |= (uint8_t)(((unsigned)*s << 8) >> c);
    d++;
    s += stride;
  }
  return d;
}

// W = (w >> 3) + 1 columns from column x >> 3: save, AND the same mask into each colour-bit block, OR image bits 0-2
// (stride 3 + flags, the whole flags word), then bit 3 and the restore only when flags bit 0 is set. With bit 0 clear
// the routine composes the buffer and leaves the screen alone (the game never asks for that).
void koei_3f26(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* and_mask, const uint8_t* or_image, uint8_t* buffer, uint16_t flags) {
  auto const at{ ColumnAt(static_cast<uint16_t>(x >> 3), y) };
  uint16_t   columns = (uint16_t)((w >> 3) + 1);
  uint16_t   stride  = (uint16_t)(3 + flags);
  unsigned   c       = x & 7;
  uint8_t*   d       = buffer;
  int        p;
  SaveRect(at, columns, h, buffer);
  for (p = 0; p < 4; p++) d = AndShifted(d, and_mask, c, columns, h);
  d = buffer;
  for (p = 0; p < 3; p++) d = OrShifted(d, or_image + p, c, columns, h, stride);
  if (!(flags & 1)) return;
  OrShifted(d, or_image + 3, c, columns, h, stride);
  RestoreRect(at, columns, h, buffer);
}
