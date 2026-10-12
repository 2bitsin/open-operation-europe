// The screen as indexed pixels and the game library's primitives on it. Eight pixels at a time travel as the bytes of
// a 64-bit word (the leftmost lowest), so a column of the game's images, a byte of a glyph and a byte of a save buffer
// each map to one word.
#include "canvas.hpp"

#include <algorithm>
#include <bit>
#include <ranges>
#include <utility>

namespace opeu::services::detail::canvas {

namespace {

static_assert(std::endian::native == std::endian::little, "pixel words are little-endian");

constexpr std::uint64_t ONES{ 0x0101010101010101 };

// Byte i of SPREAD[b] is bit 7 - i of b.
constexpr auto SPREAD{ [] {
  std::array<std::uint64_t, 256> spread{};
  for (auto const [b, word] : std::views::zip(std::views::iota(0), spread))
    for (auto const i : std::views::iota(0, 8)) word |= static_cast<std::uint64_t>((b >> (7 - i)) & 1) << (8 * i);
  return spread;
}() };

// 0xFF in the byte of every pixel whose bit is 1.
[[nodiscard]] constexpr auto Expand(std::uint8_t bits) noexcept -> std::uint64_t {
  return SPREAD[bits] * 0xFF;
}

// 0xFF in the byte of every pixel that is not colour 0.
[[nodiscard]] constexpr auto NonZero(std::uint64_t pixels) noexcept -> std::uint64_t {
  return ((pixels | pixels >> 1 | pixels >> 2 | pixels >> 3) & ONES) * 0xFF;
}

// Bit `plane` of 8 pixels, the leftmost in bit 7.
[[nodiscard]] constexpr auto Gather(std::uint64_t pixels, int plane) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>((((pixels >> plane) & ONES) * 0x8040201008040201) >> 56);
}

// The 8 colours of an image column given as one byte per colour bit.
template <std::size_t PLANES>
[[nodiscard]] constexpr auto Colours(std::span<std::uint8_t const, PLANES> column) noexcept -> std::uint64_t {
  auto const three{ SPREAD[column[0]] | SPREAD[column[1]] << 1 | SPREAD[column[2]] << 2 };
  if constexpr (PLANES == 4) return three | SPREAD[column[3]] << 3;
  return three;
}

[[nodiscard]] constexpr auto Solid(int colour) noexcept -> std::uint64_t {
  return static_cast<std::uint64_t>(colour & 0x0F) * ONES;
}

// Keeps `pixels` where `mask` is 0xFF and `under` elsewhere.
[[nodiscard]] constexpr auto Select(std::uint64_t mask, std::uint64_t pixels, std::uint64_t under) noexcept
    -> std::uint64_t {
  return (pixels & mask) | (under & ~mask);
}

// Consecutive pieces of `size` elements (the standard library here has no views::chunk).
template <typename T>
[[nodiscard]] constexpr auto Chunks(std::span<T> elements, std::size_t size) noexcept {
  return std::views::iota(0uz, elements.size() / size)
         | std::views::transform([elements, size](std::size_t i) { return elements.subspan(i * size, size); });
}

// Consecutive pieces of N elements.
template <std::size_t N, typename T>
[[nodiscard]] constexpr auto FixedChunks(std::span<T> elements) noexcept {
  return std::views::iota(0uz, elements.size() / N)
         | std::views::transform([elements](std::size_t i) { return elements.subspan(i * N).template first<N>(); });
}

// Eight neighbouring pixels of the screen, read and written as one word.
using Octet = std::span<Pixel, 8>;

[[nodiscard]] auto Get(std::span<Pixel const, 8> octet) noexcept -> std::uint64_t {
  std::array<Pixel, 8> pixels{};
  std::ranges::copy(octet, pixels.begin());
  return std::bit_cast<std::uint64_t>(pixels);
}

auto Set(Octet octet, std::uint64_t pixels) noexcept -> void {
  std::ranges::copy(std::bit_cast<std::array<Pixel, 8>>(pixels), octet.begin());
}

// The whole octets of a run of pixels.
template <typename T>
[[nodiscard]] auto Octets(std::span<T> run) noexcept {
  return std::views::iota(0uz, run.size() / 8)
         | std::views::transform([run](std::size_t i) { return run.subspan(i * 8).template first<8>(); });
}

[[nodiscard]] constexpr auto Low4(int colour) noexcept -> Pixel {
  return static_cast<Pixel>(colour & 0x0F);
}

// The rectangle (x, w in 8-pixel columns, y, h in game lines) lies inside the game area and is not empty.
[[nodiscard]] constexpr auto Clip(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h) noexcept
    -> Drawn {
  if (x > 79 || w == 0 || x + w > 80) return std::unexpected{ DrawError::OUTSIDE };
  if (y >= GAME_LINES || h == 0 || y + h > GAME_LINES) return std::unexpected{ DrawError::OUTSIDE };
  return {};
}

// The colour map the graphics mode starts with: colour 6 shows entry 0x14 (brown), colours 8-15 entries 0x38-0x3F.
constexpr std::array<std::uint8_t, COLOURS> START_COLOUR_MAP{ 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x14, 0x07,
                                                              0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F };

// The first pixel of picture unit `unit` (4 pixels, 160 per line from game line 0).
[[nodiscard]] constexpr auto UnitAt(std::uint16_t unit) noexcept -> Offset {
  return GameAt(0, 0) + Offset{ unit } * 4;
}

[[nodiscard]] auto ReadWord(ByteReader read) -> std::uint16_t {
  std::uint16_t const low{ read() };
  return static_cast<std::uint16_t>(low | read() << 8);
}

[[nodiscard]] constexpr auto Abs16(std::uint16_t v) noexcept -> std::uint16_t {
  return static_cast<std::int16_t>(v) < 0 ? static_cast<std::uint16_t>(-v) : v;
}

}

// ---- modes and colours ---------------------------------------------------------------------------------------------

Canvas::Canvas() noexcept {
  Reset(false);
}

// Black, the start colour map and the start palette: entries 0-63 the 64 colours of two bits per primary (entry bits
// 2/5 red, 1/4 green, 0/3 blue at two thirds and one third of full level), entries 64-255 black.
auto Canvas::Reset(bool shown) noexcept -> void {
  _pixels.fill(0);
  _colour_map = START_COLOUR_MAP;
  _palette.fill(0);
  for (auto const entry : std::views::iota(0, 64)) {
    auto const level{ [entry](int high, int low) {
      return static_cast<std::uint8_t>(((entry >> high) & 1) * 0x2A + ((entry >> low) & 1) * 0x15);
    } };
    SetPaletteEntry(static_cast<std::uint8_t>(entry), level(2, 5), level(1, 4), level(0, 3));
  }
  _shown   = shown;
  _changed = true;
}

auto Canvas::Start() noexcept -> void {
  _pen             = {};
  _picture_runs    = {};
  _graphics_before = false;
  Reset(true);
}

auto Canvas::EnterGraphics() noexcept -> void {
  _graphics_before = _shown;
  Reset(true);
}

auto Canvas::RestoreMode() noexcept -> void {
  Reset(_graphics_before);
}

auto Canvas::MapColour(std::uint8_t colour, std::uint8_t entry) noexcept -> void {
  _colour_map[colour & 0x0F] = static_cast<std::uint8_t>(entry & 0x3F);
  _changed                   = true;
}

auto Canvas::SetPaletteEntry(std::uint8_t entry, std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept
    -> void {
  auto const rgb{ std::span{ _palette }.subspan(entry * 3uz, 3) };
  rgb[0]   = static_cast<std::uint8_t>(red & 0x3F);
  rgb[1]   = static_cast<std::uint8_t>(green & 0x3F);
  rgb[2]   = static_cast<std::uint8_t>(blue & 0x3F);
  _changed = true;
}

// ---- pixels --------------------------------------------------------------------------------------------------------

auto Canvas::Row(int line) const noexcept -> std::span<canvas::Pixel const, WIDTH> {
  return std::span{ _pixels }.subspan(static_cast<std::size_t>(line) * WIDTH).first<WIDTH>();
}

auto Canvas::Run(Offset at, std::int64_t count) noexcept -> std::span<canvas::Pixel> {
  if (at < 0 || at >= PIXELS || count <= 0) return {};
  return std::span{ _pixels }.subspan(static_cast<std::size_t>(at),
                                      static_cast<std::size_t>(std::min<std::int64_t>(count, PIXELS - at)));
}

auto Canvas::Run(Offset at, std::int64_t count) const noexcept -> std::span<canvas::Pixel const> {
  return const_cast<Canvas*>(this)->Run(at, count);
}

auto Canvas::Load(Offset at) const noexcept -> std::uint64_t {
  std::array<canvas::Pixel, 8> group{};
  auto const                   run{ Run(at, 8) };
  if (run.size() == 8)
    std::ranges::copy(run.first<8>(), group.begin());
  else
    std::ranges::copy(run, group.begin());
  return std::bit_cast<std::uint64_t>(group);
}

auto Canvas::Store(Offset at, std::uint64_t pixels) noexcept -> void {
  auto const group{ std::bit_cast<std::array<canvas::Pixel, 8>>(pixels) };
  auto const run{ Run(at, 8) };
  if (run.size() == 8)
    std::ranges::copy(group, run.first<8>().begin());
  else
    std::ranges::copy(group | std::views::take(run.size()), run.begin());
  _changed = true;
}

auto Canvas::Pixel(Offset at) const noexcept -> canvas::Pixel {
  auto const run{ Run(at, 1) };
  return run.empty() ? canvas::Pixel{ 0 } : run.front();
}

auto Canvas::Put(Offset at, canvas::Pixel colour) noexcept -> void {
  auto const run{ Run(at, 1) };
  if (run.empty()) return;
  run.front() = Low4(colour);
  _changed    = true;
}

auto Canvas::Clear() noexcept -> void {
  _pixels.fill(0);
  _changed = true;
}

auto Canvas::PlaneBits(Offset at, int plane) const noexcept -> std::uint8_t {
  return Gather(Load(at), plane);
}

auto Canvas::Column(Offset at) const noexcept -> std::array<std::uint8_t, 4> {
  auto const pixels{ Load(at) };
  return { Gather(pixels, 0), Gather(pixels, 1), Gather(pixels, 2), Gather(pixels, 3) };
}

auto Canvas::SetColumn(Offset at, std::array<std::uint8_t, 4> const& bits) noexcept -> void {
  Store(at, Colours(std::span<std::uint8_t const, 4>{ bits }));
}

auto Canvas::SetPlaneBits(Offset at, int plane, std::uint8_t bits, std::uint8_t mask) noexcept -> void {
  auto const bit{ ONES << plane };
  auto const set{ Expand(static_cast<std::uint8_t>(bits & mask)) & bit };
  auto const clear{ Expand(static_cast<std::uint8_t>(~bits & mask)) & bit };
  Store(at, (Load(at) | set) & ~clear);
}

// ---- the pen -------------------------------------------------------------------------------------------------------

// 0 set, 1 XOR, 2 AND, 3 OR; any other code keeps the pen (and its colour).
auto Canvas::SelectPen(std::uint8_t op, std::uint8_t colour) noexcept -> void {
  static constexpr std::array OPS{ PenOp::SET, PenOp::XOR, PenOp::AND, PenOp::OR };
  if (op >= OPS.size()) return;
  _pen = { OPS[op], colour };
}

// Set, XOR and OR draw the colour where the bit is 1; AND clears the colour's bits where the bit is 0.
auto Canvas::Pen::Apply(std::uint64_t pixels, std::uint8_t bits) const noexcept -> std::uint64_t {
  auto const ink{ Expand(bits) & Solid(colour) };
  switch (op) {
  case PenOp::SET:  return Select(Expand(bits), ink, pixels);
  case PenOp::XOR:  return pixels ^ ink;
  case PenOp::AND:  return pixels & ~(~Expand(bits) & Solid(colour));
  case PenOp::OR:   return pixels | ink;
  case PenOp::NONE: break;
  }
  return pixels;
}

auto Canvas::PenBits(Offset at, std::uint8_t bits) noexcept -> void {
  Store(at, _pen.Apply(Load(at), bits));
}

// ---- images --------------------------------------------------------------------------------------------------------

// An image of `planes` bytes per 8-pixel column. 4 bytes: mode & 7FFF 0 copy, 1 XOR, 2 AND, 3 OR, 4 transparent copy
// (colour 0 keeps the screen). 3 bytes (bit 3 not stored): mode & 3FFF, copy with bit 3 = 0 (1 with mode bit 14), the
// logical modes keep bit 3, the transparent copy clears it under the drawn pixels. Copy with mode bit 15 draws every
// row on two lines (the clip checks h only).
auto Canvas::DrawColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode,
                         std::span<std::uint8_t const> image, int planes) noexcept -> Drawn {
  auto const op{ mode & (planes == 4 ? 0x7FFF : 0x3FFF) };
  if (auto const inside{ Clip(x, y, w, h) }; !inside) return inside;
  if (op > 4) return std::unexpected{ DrawError::BAD_MODE };
  auto const passes{ (op == 0 && (mode & 0x8000)) ? 2 : 1 };
  auto const unstored{ planes == 3 ? Solid(0x08) : 0 }; // colour bit 3 when the image does not hold it
  auto const fill{ (op == 0 && (mode & 0x4000) && planes == 3) ? Solid(0x08) : 0 };
  // One pass over the image per mode, so the mode is chosen once per call.
  auto const rows{ [&]<std::size_t PLANES>(auto apply) {
    auto line{ y + GAME_TOP };
    for (auto const source : Chunks(image.first(std::size_t{ w } * h * PLANES), std::size_t{ w } * PLANES))
      for ([[maybe_unused]] auto const pass : std::views::iota(0, passes)) {
        for (auto const [octet, bits] :
             std::views::zip(Octets(Run(At(x * 8, line), w * 8)), FixedChunks<PLANES>(source)))
          Set(octet, apply(Get(octet), Colours(bits)));
        line++;
      }
  } };
  auto const draw{ [&](auto apply) {
    if (planes == 4)
      rows.template operator()<4>(apply);
    else
      rows.template operator()<3>(apply);
  } };
  switch (op) {
  case 0: draw([fill](std::uint64_t, std::uint64_t colours) { return colours | fill; }); break;
  case 1: draw([](std::uint64_t under, std::uint64_t colours) { return under ^ colours; }); break;
  case 2: draw([unstored](std::uint64_t under, std::uint64_t colours) { return under & (colours | unstored); }); break;
  case 3: draw([](std::uint64_t under, std::uint64_t colours) { return under | colours; }); break;
  default:
    draw([](std::uint64_t under, std::uint64_t colours) { return Select(NonZero(colours), colours, under); });
    break;
  }
  _changed = true;
  return {};
}

// w bytes per row. Low byte of mode the colour; (mode & 7FFF) >> 8: 0 opaque (0 bits colour 0), 4 set, 1-3 the pen
// operations, others the previous pen. Bit 15 doubles rows.
auto Canvas::DrawBits(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode,
                      std::span<std::uint8_t const> bits) noexcept -> Drawn {
  auto const op{ static_cast<std::uint8_t>((mode & 0x7FFF) >> 8) };
  if (auto const inside{ Clip(x, y, w, h) }; !inside) return inside;
  if (op != 0) SelectPen(op == 4 ? 0 : op, static_cast<std::uint8_t>(mode));
  auto const passes{ (mode & 0x8000) ? 2 : 1 };
  auto       line{ y + GAME_TOP };
  for (auto const source : Chunks(bits.first(std::size_t{ w } * h), w))
    for ([[maybe_unused]] auto const pass : std::views::iota(0, passes)) {
      for (auto const [octet, byte] : std::views::zip(Octets(Run(At(x * 8, line), w * 8)), source))
        Set(octet, op == 0 ? Expand(byte) & Solid(mode) : _pen.Apply(Get(octet), byte));
      line++;
    }
  _changed = true;
  return {};
}

// 1 bits colour (mode & 15), 0 bits colour ((mode >> 8) & 15).
auto Canvas::DrawBitsOnPaper(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode,
                             std::span<std::uint8_t const> bits) noexcept -> Drawn {
  if (auto const inside{ Clip(x, y, w, h) }; !inside) return inside;
  auto line{ y + GAME_TOP };
  for (auto const source : Chunks(bits.first(std::size_t{ w } * h), w)) {
    for (auto const [octet, byte] : std::views::zip(Octets(Run(At(x * 8, line), w * 8)), source))
      Set(octet, Select(Expand(byte), Solid(mode), Solid(mode >> 8)));
    line++;
  }
  _changed = true;
  return {};
}

// x and w in pixels, rows of ceil(w / 8) bytes, all bits of the source bytes drawn with the pen over whole 8-pixel
// columns from pixel x & ~7; the pixels of those columns outside the image get a 0 bit (a 1 bit for AND).
auto Canvas::DrawShiftedBits(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode,
                             std::span<std::uint8_t const> bits) noexcept -> Drawn {
  if (x >= WIDTH || w == 0 || x + w > WIDTH) return std::unexpected{ DrawError::OUTSIDE };
  if (y >= GAME_LINES || h == 0 || y + h > GAME_LINES) return std::unexpected{ DrawError::OUTSIDE };
  auto op{ static_cast<std::uint8_t>(mode >> 8) };
  if (op == 4) op = 0;
  auto const outside{ static_cast<std::uint8_t>(op == 2 ? 0xFF : 0x00) };
  SelectPen(op, static_cast<std::uint8_t>(mode));
  auto const bytes{ ((w - 1uz) >> 3) + 1 };
  auto const shift{ x & 7 };
  auto const columns{ bytes + (shift ? 1 : 0) };
  auto       line{ y };
  for (auto const source : Chunks(bits.first(bytes * h), bytes)) {
    // The row framed by the outside bits; each drawn byte straddles two neighbours.
    std::array<std::uint8_t, 82> framed{};
    framed.front() = outside;
    std::ranges::copy(source, framed.begin() + 1);
    framed[bytes + 1] = outside;
    auto const base{ GameAt(x & ~7, line) };
    for (auto const [column, left, right] :
         std::views::zip(std::views::iota(0), framed, framed | std::views::drop(1)) | std::views::take(columns))
      PenBits(base + column * 8, static_cast<std::uint8_t>((left << 8 | right) >> shift));
    line++;
  }
  return {};
}

// A 4-byte-column image from pixel x, colour 0 transparent, unclipped. The transparent pixels of the whole columns
// after a row's first take the screen pixels that the row's first column had before drawing (at the same position in
// the column), not their own; only the trailing partial column keeps its own. w = 0 draws 65536 columns, h = 0 65536
// rows.
auto Canvas::DrawShiftedColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h,
                                std::span<std::uint8_t const> image) noexcept -> void {
  auto const shift{ x & 7 };
  auto const columns{ static_cast<std::uint16_t>(w - 1) + 1uz };
  auto const rows{ h ? std::size_t{ h } : 0x10000uz };
  auto const shifted{ [shift](std::uint64_t previous, std::uint64_t current) {
    return shift ? current << (8 * shift) | previous >> (8 * (8 - shift)) : current;
  } };
  auto line{ y };
  for (auto const source : Chunks(image.first(columns * rows * 4), columns * 4)) {
    auto const    base{ GameAt(x & ~7, line) };
    auto const    first{ Load(base) };
    std::uint64_t previous{};
    for (auto const [column, bits] : std::views::zip(std::views::iota(0), FixedChunks<4>(source))) {
      auto const pixels{ shifted(previous, Colours(bits)) };
      Store(base + column * 8, Select(NonZero(pixels), pixels, first));
      previous = Colours(bits);
    }
    if (shift) {
      auto const at{ base + static_cast<Offset>(columns) * 8 };
      auto const pixels{ shifted(previous, 0) };
      Store(at, Select(NonZero(pixels), pixels, Load(at)));
    }
    line++;
  }
}

// The column rectangle in the 4-byte-column format, unclipped; w = 0 or h = 0 saves nothing.
auto Canvas::SaveColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h,
                         std::span<std::uint8_t> image) const noexcept -> void {
  if (!w || !h) return;
  auto line{ y };
  for (auto const target : Chunks(image.first(std::size_t{ w } * h * 4), std::size_t{ w } * 4)) {
    for (auto const [column, bits] : std::views::zip(std::views::iota(int{ x }), Chunks(target, 4)))
      std::ranges::copy(Column(GameAt(column * 8, line)), bits.begin());
    line++;
  }
}

// ---- fills ---------------------------------------------------------------------------------------------------------

auto Canvas::Box(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t colour) noexcept
    -> Drawn {
  if (auto const inside{ Clip(x, y, w, h) }; !inside) return inside;
  for (auto const line : std::views::iota(y + GAME_TOP, y + GAME_TOP + h))
    std::ranges::fill(Run(At(x * 8, line), w * 8), Low4(colour));
  _changed = true;
  return {};
}

// XOR (high byte 1), AND (2: no change) or OR (3) the colour over a box, selecting that pen.
auto Canvas::Highlight(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h,
                       std::uint16_t colour) noexcept -> Drawn {
  auto const op{ static_cast<std::uint8_t>(colour >> 8) };
  if (auto const inside{ Clip(x, y, w, h) }; !inside) return inside;
  if (op == 0 || op >= 4) return std::unexpected{ DrawError::BAD_MODE };
  SelectPen(op, static_cast<std::uint8_t>(colour));
  for (auto const line : std::views::iota(y + GAME_TOP, y + GAME_TOP + h))
    for (auto const octet : Octets(Run(At(x * 8, line), w * 8))) Set(octet, _pen.Apply(Get(octet), 0xFF));
  _changed = true;
  return {};
}

// Pixels x1..x2, y1..y2 inclusive: whole 8-pixel columns between the edge columns, the edge columns under masks.
// Unclipped: x2 < x1 or y2 < y1 run on through the 16-bit counts.
auto Canvas::Fill(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t colour) noexcept
    -> void {
  auto       left{ static_cast<std::uint8_t>(0xFF >> (x1 & 7)) };
  auto       right{ static_cast<std::uint8_t>(~(0xFF >> ((x2 & 7) + 1))) };
  auto       middle{ static_cast<std::uint16_t>((x2 >> 3) - (x1 >> 3)) };
  auto const rows{ static_cast<std::uint16_t>(y2 - y1 + 1) };
  if (middle == 0) {
    left   &= right;
    right   = 0;
    middle  = 1;
  }
  middle--;
  auto const masked{ [this, colour](Offset at, std::uint8_t mask) {
    Store(at, Select(Expand(mask), Solid(colour), Load(at)));
  } };
  auto line{ static_cast<std::uint16_t>(y1 + GAME_TOP) };
  for ([[maybe_unused]] auto const row : std::views::iota(0, rows ? int{ rows } : 0x10000)) {
    auto const at{ At((x1 >> 3) * 8, line) };
    masked(at, left);
    std::ranges::fill(Run(at + 8, middle * 8), Low4(colour));
    masked(at + 8 + middle * 8, right);
    line++;
  }
}

// ---- lines ---------------------------------------------------------------------------------------------------------

// Plots game pixel (x, y) (dropped outside the game area). Colour word: low nibble colour, bits 4-7 operation (0 set,
// 1 XOR, 2 AND, 3 OR, 4 OR on pattern bits only, 5-15 set), high byte dash pattern. A 0 pattern bit draws colour 0
// with the operation, or nothing with operation 4; the pattern advances on every point inside the game area.
auto Canvas::PlotPatterned(std::uint16_t x, std::uint16_t y, std::uint16_t word, std::uint8_t& pattern_bit) noexcept
    -> void {
  auto       colour{ static_cast<std::uint8_t>(word) };
  auto const pattern{ static_cast<std::uint8_t>(word >> 8) };
  if (y >= GAME_LINES || x >= WIDTH) return;
  auto on{ true };
  if (pattern) {
    auto const bit{ 0x80 >> pattern_bit };
    pattern_bit = static_cast<std::uint8_t>((pattern_bit + 1) & 7);
    if (!(bit & pattern)) {
      colour &= 0xF0;
      on      = false;
    }
  }
  auto const at{ GameAt(x, y) };
  auto const ink{ Low4(colour) };
  switch (colour >> 4) {
  case 1: Put(at, Pixel(at) ^ ink); break;
  case 2: Put(at, Pixel(at) & ink); break;
  case 3: Put(at, Pixel(at) | ink); break;
  case 4:
    if (on) Put(at, Pixel(at) | ink);
    break;
  default: Put(at, ink); break;
  }
}

auto Canvas::Plot(std::uint16_t x, std::uint16_t y, std::uint16_t colour) noexcept -> void {
  std::uint8_t pattern_bit{};
  PlotPatterned(x, y, static_cast<std::uint16_t>(colour & 0xFF), pattern_bit);
}

// Bresenham from the end with the smaller major coordinate, the error starting at half the major length; the minor
// step is +1 when x and y increase together, else -1. The dash pattern starts at its first bit.
auto Canvas::Line(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t colour) noexcept
    -> void {
  auto const   dx{ Abs16(static_cast<std::uint16_t>(x2 - x1)) };
  auto const   dy{ Abs16(static_cast<std::uint16_t>(y2 - y1)) };
  auto const   sx1{ static_cast<std::int16_t>(x1) }, sx2{ static_cast<std::int16_t>(x2) };
  auto const   sy1{ static_cast<std::int16_t>(y1) }, sy2{ static_cast<std::int16_t>(y2) };
  auto const   step{ ((sx1 < sx2 && sy2 > sy1) || (sx1 > sx2 && sy2 < sy1)) ? 1 : -1 };
  auto const   x_major{ dx >= dy };
  auto const   major{ x_major ? dx : dy }, minor{ x_major ? dy : dx };
  auto const   from_first{ x_major ? x1 < x2 : y1 < y2 };
  auto         x{ from_first ? x1 : x2 }, y{ from_first ? y1 : y2 };
  auto         error{ static_cast<std::uint16_t>(major >> 1) };
  auto&        along{ x_major ? x : y };
  auto&        across{ x_major ? y : x };
  std::uint8_t pattern_bit{};
  for ([[maybe_unused]] auto const point : std::views::iota(0, major + 1)) {
    PlotPatterned(x, y, colour, pattern_bit);
    if (error < minor) {
      error  = static_cast<std::uint16_t>(error - minor + major);
      across = static_cast<std::uint16_t>(across + step);
    } else {
      error = static_cast<std::uint16_t>(error - minor);
    }
    along++;
  }
}

// ---- screen copy ---------------------------------------------------------------------------------------------------

// Column by column (80 per screen line); when the destination is not before the source, from the last column
// backwards, so an overlapping copy moves the block intact. Unclipped; h = 0 copies 65536 rows.
auto Canvas::CopyColumns(std::uint16_t x1, std::uint16_t y1, std::uint16_t w, std::uint16_t h, std::uint16_t x2,
                         std::uint16_t y2) noexcept -> void {
  auto       from{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(y1 + GAME_TOP) * 80u + x1) };
  auto       to{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(y2 + GAME_TOP) * 80u + x2) };
  auto       step{ static_cast<std::uint16_t>(80 - w) };
  auto const backward{ !(to < from) };
  if (backward) {
    auto const last{ static_cast<std::uint16_t>(80u * static_cast<std::uint16_t>(h - 1) + w - 1) };
    from = static_cast<std::uint16_t>(from + last);
    to   = static_cast<std::uint16_t>(to + last);
    step = static_cast<std::uint16_t>(w - 80);
  }
  for ([[maybe_unused]] auto const row : std::views::iota(0, h ? int{ h } : 0x10000)) {
    for ([[maybe_unused]] auto const column : std::views::iota(0, int{ w })) {
      Store(Offset{ to } * 8, Load(Offset{ from } * 8));
      from = static_cast<std::uint16_t>(backward ? from - 1 : from + 1);
      to   = static_cast<std::uint16_t>(backward ? to - 1 : to + 1);
    }
    from = static_cast<std::uint16_t>(from + step);
    to   = static_cast<std::uint16_t>(to + step);
  }
}

// ---- pictures ------------------------------------------------------------------------------------------------------

// A unit value holds one nibble per colour bit (nibble p = bit p of the 4 pixels, the leftmost in the nibble's bit 3).
auto Canvas::PutUnit(std::uint16_t unit, std::uint16_t value) noexcept -> void {
  for (auto const [j, pixel] : std::views::zip(std::views::iota(0), Run(UnitAt(unit), 4)))
    pixel = static_cast<canvas::Pixel>(((value >> (3 - j)) & 1) | ((value >> (6 - j)) & 2) | ((value >> (9 - j)) & 4)
                                       | ((value >> (12 - j)) & 8));
  _changed = true;
}

auto Canvas::UnitValue(std::uint16_t unit) const noexcept -> std::uint16_t {
  std::uint16_t value{};
  for (auto const [j, pixel] : std::views::zip(std::views::iota(0), Run(UnitAt(unit), 4)))
    value |= static_cast<std::uint16_t>(((pixel & 1) << (3 - j)) | ((pixel & 2) << (6 - j)) | ((pixel & 4) << (9 - j))
                                        | ((pixel & 8) << (12 - j)));
  return value;
}

auto Canvas::MaskedLiteral(std::uint8_t command, ByteReader read) const noexcept -> std::uint16_t {
  auto const kind{ static_cast<std::uint8_t>(command & 0x78) };
  if (kind == 0) return 0;
  if (kind & 0x40) return _picture_runs[(kind >> 3) & 7];
  if (kind == 0x38) {
    std::uint16_t const high{ read() };
    return static_cast<std::uint16_t>(high << 8 | read());
  }
  auto const c{ read() };
  switch ((kind - 8) >> 3) {
  case 0:  return c;
  case 1:  return static_cast<std::uint16_t>((c & 0xF0) << 4 | (c & 0x0F));
  case 2:  return static_cast<std::uint16_t>((c & 0xF0) << 8 | (c & 0x0F));
  case 3:  return static_cast<std::uint16_t>(c << 4);
  case 4:  return static_cast<std::uint16_t>((c & 0xF0) << 8 | (c & 0x0F) << 4);
  default: return static_cast<std::uint16_t>(c << 8);
  }
}

// Pictures are coded in units of 4 pixels from game line 0, 160 units per line. Stream:
//   width (word, 1..640), height (word, 1..400), y + height <= 400, x + width <= 640 (x truncated to a unit);
//   [masked only] 8 words: the run-colour table, kept between pictures;
//   then per row, commands until (width + 3) / 4 units are covered (the last may run past the row end; the next row
//   starts at the line start):
//   1sxxcccc  copy c+1 units from xx+1 units back (s = 0) or xx+1 rows up (s = 1), unit by unit (overlap repeats);
//   0.......  literal:
//     plain:  0nnnpppp qqqqqqqq -> n+1 units of value p:q (bit 3 of the colours 0);
//     masked: 0kkkkrrr, r+1 units, by kkkk: 0000 value 0; 1kkk run table entry kkk; 0111 two bytes a b, value a:b;
//             0001..0110 one byte b for the colour-bit pairs (1,0) (2,0) (3,0) (2,1) (3,1) (3,2), high nibble to the
//             higher bit, the other bits 0.
// A bad header leaves the stream partly read.
auto Canvas::DecodePicture(std::uint16_t x, std::uint16_t y, ByteReader read, bool masked) noexcept -> Drawn {
  auto const width{ ReadWord(read) };
  if (static_cast<std::uint16_t>(width - 1) > 639) return std::unexpected{ DrawError::BAD_HEADER };
  auto const height{ ReadWord(read) };
  if (static_cast<std::uint16_t>(height - 1) > 399) return std::unexpected{ DrawError::BAD_HEADER };
  if (y > 399 || static_cast<std::uint16_t>(height + y) > 400) return std::unexpected{ DrawError::BAD_HEADER };
  if (x > 639 || static_cast<std::uint16_t>(x + width) > 640) return std::unexpected{ DrawError::BAD_HEADER };
  auto const units{ static_cast<std::uint16_t>((width + 3) >> 2) };
  if (masked) std::ranges::generate(_picture_runs, [read] { return ReadWord(read); });
  for (auto const row : std::views::iota(0, int{ height })) {
    auto unit{ static_cast<std::uint16_t>((x >> 2) + (y + row) * 160u) };
    // Commands until the row's units are covered; the decoder cannot know a command's length before reading it.
    for (auto left{ int{ units } }; left > 0;) {
      auto const    command{ read() };
      std::uint16_t n{};
      if (!(command & 0x80)) {
        n = static_cast<std::uint16_t>(masked ? (command & 7) + 1 : ((command >> 4) & 7) + 1);
        auto const value{ masked ? MaskedLiteral(command, read)
                                 : static_cast<std::uint16_t>((command & 0x0F) << 8 | read()) };
        for (auto const i : std::views::iota(0, int{ n })) PutUnit(static_cast<std::uint16_t>(unit + i), value);
      } else {
        auto back{ static_cast<std::uint16_t>(((command & 0x30) >> 4) + 1) };
        if (command & 0x40) back = static_cast<std::uint16_t>(back * 160u);
        n = static_cast<std::uint16_t>((command & 0x0F) + 1);
        auto const from{ static_cast<std::uint16_t>(unit - back) };
        for (auto const i : std::views::iota(0, int{ n }))
          PutUnit(static_cast<std::uint16_t>(unit + i), UnitValue(static_cast<std::uint16_t>(from + i)));
      }
      unit  = static_cast<std::uint16_t>(unit + n);
      left -= n;
    }
  }
  return {};
}

// ---- presentation --------------------------------------------------------------------------------------------------

auto Canvas::TakeChanged() noexcept -> bool {
  return std::exchange(_changed, false);
}

auto Canvas::Render(std::span<std::uint8_t, PIXELS>        frame,
                    std::span<std::uint8_t, PALETTE_BYTES> palette) const noexcept -> void {
  std::ranges::transform(_palette, palette.begin(),
                         [](std::uint8_t v) { return static_cast<std::uint8_t>(v << 2 | v >> 4); });
  if (!_shown) {
    std::ranges::fill(frame, 0);
    return;
  }
  std::ranges::transform(_pixels, frame.begin(), [this](canvas::Pixel pixel) { return _colour_map[pixel]; });
}

auto Screen() noexcept -> Canvas& {
  static Canvas screen;
  return screen;
}

}
