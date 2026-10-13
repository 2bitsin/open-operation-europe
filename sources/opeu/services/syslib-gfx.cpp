// The game's battle-cell and sprite routines (SYSLIB drawing) on the canvas. The cell buffers and sprite images stay in
// the game's form, one byte per colour bit for 8 pixels; the screen side reads and writes those bits of the pixels.
#include "syslib-gfx.hpp"

#include "canvas.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>

namespace {

using opeu::services::GameAt;
using opeu::services::Offset;
using opeu::services::Screen;

using Cell      = std::span<std::uint8_t, SYSLIB_CELL_BYTES>;
using ConstCell = std::span<std::uint8_t const, SYSLIB_CELL_BYTES>;

constexpr auto CELL_LINES{ 26uz };
constexpr auto CELL_COLUMNS{ 5uz };
constexpr auto LINE_BYTES{ 20uz }; // the 5 columns of colour bit 0, then of bits 1, 2 and 3
constexpr auto MASK_BYTES{ CELL_LINES * CELL_COLUMNS };
constexpr auto GLYPH_LINES{ 6uz };
constexpr auto LINE_PIXELS{ Offset{ 640 } };

// The canvas position of column x (8 pixels) on game line y; columns past 80 continue on the next line.
[[nodiscard]] auto ColumnAt(std::uint32_t const x, std::int64_t const y) noexcept -> Offset {
  return GameAt(std::int64_t{ x } * 8, y);
}

// A loop count of 0 runs 65536 times.
[[nodiscard]] auto LoopCount(std::uint32_t const n) noexcept -> std::size_t {
  return n != 0 ? n : 0x10000uz;
}

[[nodiscard]] auto Low8(std::uint32_t const value) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(value);
}

// Visits each (line, column) of a `lines` x `columns` rectangle, line by line (libc++ has no views::cartesian_product).
auto EachPlace(std::size_t const lines, std::size_t const columns, auto const& visit) noexcept -> void {
  std::ranges::for_each(std::views::iota(0uz, lines), [&](std::size_t const line) { std::ranges::for_each(std::views::iota(0uz, columns), [&](std::size_t const column) { visit(line, column); }); });
}

// ---- cell <-> screen ---------------------------------------------------------------------------------------------

// The cell byte of colour bit `plane` at column `column` of line `line`.
[[nodiscard]] constexpr auto CellIndex(std::size_t const line, std::size_t const plane, std::size_t const column) noexcept -> std::size_t {
  return line * LINE_BYTES + plane * CELL_COLUMNS + column;
}

// The screen position of the cell's column `column` on line `line`, the cell starting at `at`.
[[nodiscard]] auto CellColumnAt(Offset const at, std::size_t const line, std::size_t const column) noexcept -> Offset {
  return at + Offset(line) * LINE_PIXELS + Offset(column) * 8;
}

auto SaveCell(Offset const at, Cell const cell) noexcept -> void {
  EachPlace(CELL_LINES, CELL_COLUMNS, [&](std::size_t const line, std::size_t const column) {
    auto const bits{ Screen().Column(CellColumnAt(at, line, column)) };
    std::ranges::for_each(std::views::iota(0uz, bits.size()), [cell, bits, line, column](std::size_t const plane) { cell[CellIndex(line, plane, column)] = bits[plane]; });
  });
}

auto RestoreCell(Offset const at, ConstCell const cell) noexcept -> void {
  EachPlace(CELL_LINES, CELL_COLUMNS, [&](std::size_t const line, std::size_t const column) {
    Screen().SetColumn(CellColumnAt(at, line, column), { cell[CellIndex(line, 0, column)], cell[CellIndex(line, 1, column)], cell[CellIndex(line, 2, column)], cell[CellIndex(line, 3, column)] });
  });
}

// ---- rectangles <-> screen ---------------------------------------------------------------------------------------

// A w-column x h-line screen rectangle (h = 0: 65536 lines) held colour-bit-major: all of bit 0, then bit 1, ...
struct Rect {
  Offset      at;
  std::size_t columns;
  std::size_t lines;

  [[nodiscard]] auto PlaneBytes() const noexcept -> std::size_t {
    return columns * lines;
  }
  [[nodiscard]] auto ScreenAt(std::size_t const line, std::size_t const column) const noexcept -> Offset {
    return CellColumnAt(at, line, column);
  }
};

auto SaveRect(Rect const& rect, std::span<std::uint8_t> const buffer) noexcept -> void {
  auto const plane_bytes{ rect.PlaneBytes() };
  EachPlace(rect.lines, rect.columns, [rect, buffer, plane_bytes](std::size_t const line, std::size_t const column) {
    auto const bits{ Screen().Column(rect.ScreenAt(line, column)) };
    std::ranges::for_each(std::views::iota(0uz, bits.size()),
                          [buffer, bits, plane_bytes, at = line * rect.columns + column](std::size_t const plane) { buffer[plane * plane_bytes + at] = bits[plane]; });
  });
}

auto RestoreRect(Rect const& rect, std::span<std::uint8_t const> const buffer) noexcept -> void {
  auto const plane_bytes{ rect.PlaneBytes() };
  EachPlace(rect.lines, rect.columns, [rect, buffer, plane_bytes](std::size_t const line, std::size_t const column) {
    auto const i{ line * rect.columns + column };
    Screen().SetColumn(rect.ScreenAt(line, column), { buffer[i], buffer[plane_bytes + i], buffer[2 * plane_bytes + i], buffer[3 * plane_bytes + i] });
  });
}

// ---- glyphs in a cell buffer -------------------------------------------------------------------------------------

// A glyph line shifted right by 0-7 pixels across a byte pair of the cell.
struct GlyphBits {
  std::uint8_t first;
  std::uint8_t second;
};

// A glyph line at a cell offset.
struct GlyphLine {
  std::size_t at;
  GlyphBits   bits;
};

// The glyph at pixel (x, y): its first line's cell offset, in 16 bits as the original computed it, and the shift.
struct GlyphPlace {
  std::size_t   start;
  std::uint32_t pixels;
};

[[nodiscard]] auto PlaceOf(std::uint32_t const x, std::uint32_t const y) noexcept -> GlyphPlace {
  return { .start = (y * LINE_BYTES + (x >> 3)) & 0xFFFF, .pixels = x & 7 };
}

[[nodiscard]] auto Shifted(std::uint8_t const line, std::uint32_t const pixels) noexcept -> GlyphBits {
  auto const word{ ((std::uint32_t{ line } << 16) | line) >> pixels };
  return { .first = Low8(word), .second = Low8(word >> 8) };
}

// The six lines of a glyph, 20 bytes apart.
[[nodiscard]] auto Lines(std::span<std::uint8_t const, GLYPH_LINES> const glyph, GlyphPlace const place) noexcept {
  return std::views::zip(std::views::iota(0uz, GLYPH_LINES), glyph) | std::views::transform([place](auto const entry) {
           auto const [line, bits]{ entry };
           return GlyphLine{ .at = place.start + line * LINE_BYTES, .bits = Shifted(bits, place.pixels) };
         });
}

// OR / AND-NOT of a byte pair of the cell buffer. A glyph in column 4 spills its second byte into column 0 of the next
// colour bit's block (or of the next line); the game's cell glyphs rely on nothing else.
auto SetBits(Cell const cell, std::size_t const at, GlyphBits const bits) noexcept -> void {
  cell[at]     |= bits.first;
  cell[at + 1] |= bits.second;
}

auto ClearBits(Cell const cell, std::size_t const at, GlyphBits const bits) noexcept -> void {
  cell[at]     &= Low8(~std::uint32_t{ bits.first });
  cell[at + 1] &= Low8(~std::uint32_t{ bits.second });
}

// ---- shifted sprite blocks ----------------------------------------------------------------------------------------

// One colour-bit block of the pixel-positioned sprite buffer: `lines` rows of `width` bytes, each byte made of a source
// row's bytes j - 1 and j (width - 1 per row) shifted right by `pixels`, `edge` shifted in at both ends.
struct ShiftedRows {
  std::size_t   lines;
  std::size_t   width;
  std::uint32_t pixels;
  std::uint8_t  edge;

  [[nodiscard]] auto Bytes() const noexcept -> std::size_t {
    return lines * width;
  }
  [[nodiscard]] auto SourceBytes() const noexcept -> std::size_t {
    return lines * (width - 1);
  }
  // Byte `column` of a row whose source bytes are `source(0)` .. `source(width - 2)`.
  [[nodiscard]] auto At(auto const& source, std::size_t const column) const noexcept -> std::uint8_t {
    auto const high{ column == 0 ? edge : source(column - 1) };
    auto const low{ column == width - 1 ? edge : source(column) };
    return Low8(((std::uint32_t{ high } << 8) | low) >> pixels);
  }
  // Combines each row of `block` with its shifted source row, `source(line)` the row's source byte accessor.
  auto Apply(std::span<std::uint8_t> const block, auto const& source, auto const& combine) const noexcept -> void {
    std::ranges::for_each(std::views::iota(0uz, lines), [&](std::size_t const line) {
      auto const row{ block.subspan(line * width, width) };
      auto const from{ source(line) };
      std::ranges::transform(row, std::views::iota(0uz, width), row.begin(),
                             [self = *this, from, combine](std::uint8_t const bits, std::size_t const column) { return combine(bits, self.At(from, column)); });
    });
  }
};

auto AndShifted(std::span<std::uint8_t> const block, ShiftedRows const& rows, std::span<std::uint8_t const> const mask) noexcept -> void {
  rows.Apply(
      block, [&](std::size_t const line) { return [row = mask.subspan(line * (rows.width - 1))](std::size_t const i) { return row[i]; }; },
      [](std::uint8_t const bits, std::uint8_t const keep) { return Low8(bits & keep); });
}

// The image interleaves the colour bits: `stride` bytes per byte column, the block's bit first.
auto OrShifted(std::span<std::uint8_t> const block, ShiftedRows const& rows, std::span<std::uint8_t const> const image, std::size_t const stride) noexcept -> void {
  rows.Apply(
      block, [&](std::size_t const line) { return [image, stride, base = line * (rows.width - 1)](std::size_t const i) { return image[(base + i) * stride]; }; },
      [](std::uint8_t const bits, std::uint8_t const more) { return Low8(bits | more); });
}

// ORs an image interleaving PLANES colour bits per byte column into the colour-bit blocks of `bytes`, `block` bytes
// apart (the blocks overlap when it is smaller than the image's columns).
template <std::size_t PLANES>
auto OrInterleaved(std::span<std::uint8_t> const bytes, std::span<std::uint8_t const> const image, std::size_t const block) noexcept -> void {
  std::ranges::for_each(std::views::iota(0uz, image.size() / PLANES), [bytes, image, block](std::size_t const i) {
    std::ranges::for_each(std::views::iota(0uz, PLANES), [bytes, image, block, i](std::size_t const plane) { bytes[plane * block + i] |= image[i * PLANES + plane]; });
  });
}

}

// ---- battle cells (26 lines x 5 columns) -------------------------------------------------------------------------

auto opeu_vram_restore(std::uint16_t const x, std::uint16_t const y, std::uint8_t const* const vram) -> void {
  RestoreCell(ColumnAt(x, y), ConstCell{ vram, SYSLIB_CELL_BYTES });
}

// The cell starts half a column right (pixel x * 8 + 4); the pixels around it keep their colours.
auto opeu_vram_restore_odd(std::uint16_t const x, std::uint16_t const y, std::uint8_t const* const vram) -> void {
  RestoreCell(ColumnAt(x, y) + 4, ConstCell{ vram, SYSLIB_CELL_BYTES });
}

auto opeu_vram_save(std::uint16_t const x, std::uint16_t const y, std::uint8_t* const vram) -> void {
  SaveCell(ColumnAt(x, y), Cell{ vram, SYSLIB_CELL_BYTES });
}

auto opeu_vram_save_odd(std::uint16_t const x, std::uint16_t const y, std::uint8_t* const vram) -> void {
  SaveCell(ColumnAt(x, y) + 4, Cell{ vram, SYSLIB_CELL_BYTES });
}

// ---- cell-buffer composition (memory only) -------------------------------------------------------------------

auto opeu_vram_draw(std::uint8_t* const vram, std::uint8_t const* const src) -> void {
  Cell const cell{ vram, SYSLIB_CELL_BYTES };
  std::ranges::transform(cell, ConstCell{ src, SYSLIB_CELL_BYTES }, cell.begin(), [](std::uint8_t const bits, std::uint8_t const more) { return Low8(bits | more); });
}

auto opeu_vram_draw_near(std::uint8_t* const vram, std::uint8_t const* const src) -> void {
  opeu_vram_draw(vram, src);
}

// A 6-line glyph at pixel (x, y) of the cell buffer in colour 2 (low byte of color 0) or colour 1. The game's glyphs
// (y at most 15, x at most 24) stay inside the cell.
auto opeu_vram_glyph_color(std::uint8_t* const vram, std::uint8_t const* const glyph, std::uint16_t const x, std::uint16_t const y, std::uint16_t const color) -> void {
  Cell const cell{ vram, SYSLIB_CELL_BYTES };
  auto const colour_one{ Low8(color) != 0 };
  std::ranges::for_each(Lines(std::span<std::uint8_t const, GLYPH_LINES>{ glyph, GLYPH_LINES }, PlaceOf(x, y)), [&](GlyphLine const line) {
    if (colour_one) {
      SetBits(cell, line.at, line.bits);
      ClearBits(cell, line.at + 5, line.bits);
    } else {
      ClearBits(cell, line.at, line.bits);
      SetBits(cell, line.at + 5, line.bits);
    }
    ClearBits(cell, line.at + 10, line.bits);
    ClearBits(cell, line.at + 15, line.bits);
  });
}

// A 6-line glyph in colour 7 over colours 0-7: bits 0-2 set, bit 3 cleared.
auto opeu_vram_glyph(std::uint8_t* const vram, std::uint8_t const* const glyph, std::uint16_t const x, std::uint16_t const y) -> void {
  Cell const cell{ vram, SYSLIB_CELL_BYTES };
  std::ranges::for_each(Lines(std::span<std::uint8_t const, GLYPH_LINES>{ glyph, GLYPH_LINES }, PlaceOf(x, y)), [&](GlyphLine const line) {
    SetBits(cell, line.at, line.bits);
    SetBits(cell, line.at + 5, line.bits);
    SetBits(cell, line.at + 10, line.bits);
    ClearBits(cell, line.at + 15, line.bits);
  });
}

// ANDs a 26x5-byte mask into the four colour-bit blocks of every line.
auto opeu_vram_load(std::uint8_t* const vram, std::uint8_t const* const mask) -> void {
  Cell const                                      cell{ vram, SYSLIB_CELL_BYTES };
  std::span<std::uint8_t const, MASK_BYTES> const masks{ mask, MASK_BYTES };
  std::ranges::transform(cell, std::views::iota(0uz, cell.size()), cell.begin(),
                         [masks](std::uint8_t const bits, std::size_t const i) { return Low8(bits & masks[i / LINE_BYTES * CELL_COLUMNS + i % CELL_COLUMNS]); });
}

// ---- screen fills and copies ------------------------------------------------------------------------------------

// Bit 3 of the colours of a w-column x h-line rectangle from the bits of the low byte of `color` (h = 0: 65536 lines);
// bits 0-2 keep. Unclipped.
auto Koei3b77(std::uint16_t const x, std::uint16_t const y, std::uint16_t const w, std::uint16_t const h, std::uint16_t const color) -> void {
  EachPlace(LoopCount(h), w, [&](std::size_t const line, std::size_t const column) { Screen().SetPlaneBits(ColumnAt((x + column) & 0xFFFF, y) + Offset(line) * LINE_PIXELS, 3, Low8(color)); });
}

// Copies the 3-column x 24-line block at (x1, y1) to (x2, y2), line by line from the top; within a line the first two
// columns are read before either is written.
auto koei_3bc5(std::uint16_t const x1, std::uint16_t const y1, std::uint16_t const x2, std::uint16_t const y2) -> void {
  auto&      screen{ Screen() };
  auto const from{ ColumnAt(x1, y1) };
  auto const to{ ColumnAt(x2, y2) };
  std::ranges::for_each(std::views::iota(0, 24) | std::views::transform([](int const line) { return Offset{ line } * LINE_PIXELS; }), [&](Offset const line) {
    screen.Move<16>(to + line, from + line);
    screen.Move<8>(to + line + 16, from + line + 16);
  });
}

// n = w * h in 16 bits; a product of 0 runs the AND and OR passes 65536 times. The image interleaves bits 0-2 (and 3
// when flags bit 0 is set).
auto opeu_sprite_compose(std::uint16_t const x, std::uint16_t const y, std::uint16_t const w, std::uint16_t const h, std::uint8_t const* const and_mask, std::uint8_t const* const or_image,
                         std::uint8_t* const buffer, std::uint16_t const flags) -> void {
  Rect const      rect{ .at = ColumnAt(x, y), .columns = w, .lines = LoopCount(h) };
  auto const      block{ (std::size_t{ w } * h) & 0xFFFF };
  auto const      count{ LoopCount(static_cast<std::uint32_t>(block)) };
  auto const      planes{ (flags & 1) != 0 ? 4uz : 3uz };
  std::span const bytes{ buffer, std::max(4 * rect.PlaneBytes(), 3 * block + count) };
  std::span const mask{ and_mask, count };
  std::span const image{ or_image, planes * count };
  SaveRect(rect, bytes);
  std::ranges::for_each(std::views::iota(0uz, 4uz), [&](std::size_t const plane) {
    auto const bits{ bytes.subspan(plane * block, count) };
    std::ranges::transform(bits, mask, bits.begin(), [](std::uint8_t const old, std::uint8_t const keep) { return Low8(old & keep); });
  });
  if (planes == 4) {
    OrInterleaved<4>(bytes, image, block);
  } else {
    OrInterleaved<3>(bytes, image, block);
  }
  RestoreRect(rect, bytes);
}

// W = (w >> 3) + 1 columns from column x >> 3: save, AND the same mask into each colour-bit block, OR image bits 0-2
// (stride 3 + flags, the whole flags word), then bit 3 and the restore only when flags bit 0 is set. With bit 0 clear
// the routine composes the buffer and leaves the screen alone (the game never asks for that). W = 1 makes a row 65537
// bytes wide (the original's middle loop runs 65535 times).
auto koei_3f26(std::uint16_t const x, std::uint16_t const y, std::uint16_t const w, std::uint16_t const h, std::uint8_t const* const and_mask, std::uint8_t const* const or_image,
               std::uint8_t* const buffer, std::uint16_t const flags) -> void {
  auto const        columns{ (std::size_t{ w } >> 3) + 1 };
  Rect const        rect{ .at = ColumnAt(std::uint32_t{ x } >> 3, y), .columns = columns, .lines = LoopCount(h) };
  auto const        width{ ((columns + 0xFFFE) & 0xFFFF) + 2 };
  auto const        stride{ 3uz + flags };
  ShiftedRows const masked{ .lines = rect.lines, .width = width, .pixels = x & 7u, .edge = 0xFF };
  ShiftedRows const drawn{ .lines = rect.lines, .width = width, .pixels = x & 7u, .edge = 0 };
  auto const        planes{ (flags & 1) != 0 ? 4uz : 3uz };
  std::span const   bytes{ buffer, 4 * masked.Bytes() };
  std::span const   mask{ and_mask, masked.SourceBytes() };
  std::span const   image{ or_image, (drawn.SourceBytes() - 1) * stride + 4 };
  SaveRect(rect, bytes);
  std::ranges::for_each(std::views::iota(0uz, 4uz), [&](std::size_t const plane) { AndShifted(bytes.subspan(plane * masked.Bytes(), masked.Bytes()), masked, mask); });
  std::ranges::for_each(std::views::iota(0uz, planes), [&](std::size_t const plane) { OrShifted(bytes.subspan(plane * drawn.Bytes(), drawn.Bytes()), drawn, image.subspan(plane), stride); });
  if ((flags & 1) == 0) return;
  RestoreRect(rect, bytes);
}
