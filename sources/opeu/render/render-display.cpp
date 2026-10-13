#include "opeu/render/render-display.hpp"

#include "opeu/assets/picture-stream.hpp"
#include "opeu/render/line-points.hpp"
#include "opeu/services/syslib-gfx.hpp"

#include <algorithm>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace opeu::render::detail::render_display {
namespace {

constexpr auto SCREEN_COLUMNS{ 80 };
constexpr auto GAME_LINES{ 400 };
constexpr auto FRAME_WIDTH{ 640 };
constexpr auto CELL_SIZE{
  Size{ .width = 40, .height = 26 }
};
constexpr auto BLOCK_SIZE{
  Size{ .width = 24, .height = 24 }
};
constexpr auto TEXTURE_CAPACITY{ 4096uz };

// Mode bits of the column images: rows drawn twice, and colour bit 3 set by a three-byte copy.
constexpr Word DOUBLED_ROWS{ 0x8000 };
constexpr Word SETS_BIT_3{ 0x4000 };

// A column rectangle (x and width in 8-pixel columns, y in game lines) inside the game area and not empty.
[[nodiscard]] constexpr auto Inside(services::Area const area) noexcept -> bool {
  return area.x < SCREEN_COLUMNS && area.width != 0 && area.x + area.width <= SCREEN_COLUMNS && area.y < GAME_LINES && area.height != 0 && area.y + area.height <= GAME_LINES;
}

// The same for a rectangle whose x and width are pixels.
[[nodiscard]] constexpr auto InsidePixels(services::Area const area) noexcept -> bool {
  return area.x < FRAME_WIDTH && area.width != 0 && area.x + area.width <= FRAME_WIDTH && area.y < GAME_LINES && area.height != 0 && area.y + area.height <= GAME_LINES;
}

// The frame pixel of game pixel (x, line).
[[nodiscard]] constexpr auto FramePoint(int const x, int const line) noexcept -> Point {
  return Point{ .x = x, .y = line + GAME_TOP };
}

[[nodiscard]] constexpr auto ColumnPoint(services::Point const at) noexcept -> Point {
  return FramePoint(at.x * 8, at.y);
}

// The largest picture that fits from game pixel `at` (none off the game area).
[[nodiscard]] auto RoomAt(services::Point const at) -> assets::Room {
  if (at.x >= FRAME_WIDTH || at.y >= GAME_LINES) return assets::Room{ .width = 0, .height = 0 };
  return assets::Room{ .width = static_cast<std::size_t>(FRAME_WIDTH - at.x), .height = static_cast<std::size_t>(GAME_LINES - at.y) };
}

// Half a column right: where a shifted battle cell lies.
[[nodiscard]] constexpr auto HalfColumnRight(Point const at) noexcept -> Point {
  return Point{ .x = at.x + 4, .y = at.y };
}

[[nodiscard]] constexpr auto ColumnRect(services::Area const area) noexcept -> Rect {
  return At(ColumnPoint({ .x = area.x, .y = area.y }), Size{ .width = area.width * 8, .height = area.height });
}

[[nodiscard]] auto BytesOf(std::span<std::uint8_t const> const bytes) -> assets::Bytes {
  return std::as_bytes(bytes);
}

// The game's column image of `area` in `planes` bytes per column.
[[nodiscard]] auto ColumnImage(services::Area const area, std::span<std::uint8_t const> const bytes, std::size_t const planes) -> GameImage {
  return GameImage{ .layout = assets::InterleavedLayout(area.width, area.height, planes), .bytes = BytesOf(bytes), .shipped = planes == 4 ? assets::Layout::PLANES_4 : assets::Layout::PLANES_3 };
}

// A one-bit image of `width_bytes` bytes per row.
[[nodiscard]] auto BitsImage(std::size_t const width_bytes, std::size_t const height, std::span<std::uint8_t const> const bytes) -> GameImage {
  return GameImage{ .layout = assets::InterleavedLayout(width_bytes, height, 1), .bytes = BytesOf(bytes), .shipped = assets::Layout::BITMAP };
}

// Colours in which number n shows the colour of n | 8: a three-byte copy that sets colour bit 3.
[[nodiscard]] auto WithBit3(assets::PaletteColours const& colours) -> assets::PaletteColours {
  auto shifted{ colours };
  std::ranges::transform(assets::Indices(colours.size()), shifted.begin(), [&colours](std::size_t const n) { return colours.at(n | 8U); });
  return shifted;
}

// The numbers of `image` with every row repeated `times` times.
[[nodiscard]] auto RowsRepeated(assets::Image const& image, std::size_t const times) -> assets::Image {
  if (times == 1) return image;
  auto const rows{ assets::Indices(image.Height() * times) | std::views::transform([&](std::size_t const row) { return image.Pixels().subspan(row / times * image.Width(), image.Width()); }) };
  return assets::Image{ image.Width(), image.Height() * times, rows | std::views::join | std::ranges::to<std::vector>() };
}

// The numbers of `under` after `rework(under number, other number)` with the numbers of `other`, pixel by pixel.
template <class _Rework>
[[nodiscard]] auto Reworked(assets::Image const& under, assets::Image const& other, _Rework rework) -> assets::Image {
  auto numbers{ std::views::zip(under.Pixels(), other.Pixels()) | std::views::transform([&rework](auto const pair) { return rework(std::get<0>(pair), std::get<1>(pair)); })
                | std::ranges::to<std::vector>() };
  return assets::Image{ under.Width(), under.Height(), std::move(numbers) };
}

// `image` placed `shift` pixels into an image `width` wide, `outside` around it.
[[nodiscard]] auto Shifted(assets::Image const& image, std::size_t const shift, std::size_t const width, std::uint8_t const outside) -> assets::Image {
  auto const pixel{ [&](std::size_t const row, std::size_t const x) { return x >= shift && x - shift < image.Width() ? image.At(x - shift, row) : outside; } };
  auto       numbers{ assets::Grid(image.Height(), width) | std::views::transform([&pixel](auto const place) { return pixel(place.first, place.second); }) | std::ranges::to<std::vector>() };
  return assets::Image{ width, image.Height(), std::move(numbers) };
}

[[nodiscard]] auto Decoded(GameImage const& image) -> assets::Image {
  return Required(assets::DecodePlanes(image.bytes, image.layout).transform_error([](assets::DecodeError const error) { return RenderError{ .detail = std::string{ assets::Describe(error) } }; }));
}

// `bytes` copied into the start of `into`, as much as it holds.
auto CopyInto(std::vector<std::byte> const& bytes, std::span<std::uint8_t> const into) -> void {
  std::ranges::transform(bytes | std::views::take(into.size()), into.begin(), [](std::byte const byte) { return std::to_integer<std::uint8_t>(byte); });
}

// The colour word of a line point: low nibble the colour, bits 4-7 the operation, high byte the dash pattern.
struct LineInk {
  std::uint8_t colour;
  std::uint8_t operation;
  std::uint8_t pattern;
};

[[nodiscard]] constexpr auto InkOf(Word const word) noexcept -> LineInk {
  return LineInk{ .colour = static_cast<std::uint8_t>(word & 0x0F), .operation = static_cast<std::uint8_t>(word >> 4 & 0x0F), .pattern = static_cast<std::uint8_t>(word >> 8) };
}

// A point of a line and whether its dash pattern bit draws.
struct DashedPoint {
  Point at;
  bool  on;
};

// The line's points on the game area, in order, each with its pattern bit (the pattern advances on those points only).
[[nodiscard]] auto DashedPoints(services::Corners const ends, std::uint8_t const pattern) -> std::vector<DashedPoint> {
  auto const inside{ [](LinePoint const point) { return point.x < FRAME_WIDTH && point.y < GAME_LINES; } };
  auto const points{ LinePoints({ .x = ends.from.x, .y = ends.from.y }, { .x = ends.to.x, .y = ends.to.y }) | std::views::filter(inside) | std::ranges::to<std::vector>() };
  return std::views::zip(points, std::views::iota(0uz)) | std::views::transform([pattern](auto const entry) {
           auto const [point, n]{ entry };
           return DashedPoint{ .at = FramePoint(point.x, point.y), .on = pattern == 0 || (pattern << (n % 8) & 0x80) != 0 };
         })
         | std::ranges::to<std::vector>();
}

// What a line point's operation makes of the number under it.
[[nodiscard]] constexpr auto LineApplied(std::uint8_t const number, LineInk const ink, bool const on) noexcept -> std::uint8_t {
  auto const colour{ on ? ink.colour : std::uint8_t{ 0 } };
  switch (ink.operation) {
  case 1:  return static_cast<std::uint8_t>(number ^ colour);
  case 2:  return static_cast<std::uint8_t>(number & colour);
  case 3:  return static_cast<std::uint8_t>(number | colour);
  case 4:  return on ? static_cast<std::uint8_t>(number | colour) : number;
  default: return colour;
  }
}

// The smallest rectangle holding every point.
[[nodiscard]] auto Bounds(std::span<DashedPoint const> const points) -> Rect {
  auto const [left, right]{ std::ranges::minmax(points | std::views::transform([](DashedPoint const& point) { return point.at.x; })) };
  auto const [top, bottom]{ std::ranges::minmax(points | std::views::transform([](DashedPoint const& point) { return point.at.y; })) };
  return Rect{ .x = left, .y = top, .width = right - left + 1, .height = bottom - top + 1 };
}

}

RenderDisplay::RenderDisplay(render::Display display, assets::AssetPack const& pack, assets::ContentIndex const& index)
    : _frame{ std::move(display), _palette.Colours() }, _pack_textures{ _frame.Renderer(), pack, TEXTURE_CAPACITY }, _textures{ _frame.Renderer(), _pack_textures, index, TEXTURE_CAPACITY } { }

// ---- the screen ----------------------------------------------------------------------------------------------------

auto RenderDisplay::EnterGraphics() -> void {
  _mode_before = _mode;
  Reset(Mode::GRAPHICS);
}

auto RenderDisplay::LeaveGraphics() -> void {
  Reset(_mode_before);
}

// Black in the start colours, shown or not.
auto RenderDisplay::Reset(Mode const mode) -> void {
  _palette = ScreenPalette{};
  _frame.Restart(_palette.Colours());
  _mode = mode;
}

auto RenderDisplay::Clear() -> void {
  _frame.Clear(0);
}

auto RenderDisplay::ShowIdentityPalette() -> void {
  _palette.ShowIdentity();
  ChangeColours();
}

auto RenderDisplay::LoadPalette(std::uint8_t const first, std::span<std::uint8_t const> const levels) -> void {
  _palette.Load(first, levels);
  ChangeColours();
}

auto RenderDisplay::ChangeColours() -> void {
  _frame.Recolour(_palette.Colours());
}

auto RenderDisplay::Paint(Keying const keying) const -> render::Paint {
  return render::Paint{ .colours = _frame.Colours(), .keying = keying };
}

// ---- images --------------------------------------------------------------------------------------------------------

auto RenderDisplay::DrawColumns(services::Area const area, services::Image const image) -> bool {
  return DrawColumnImage(area, image, 4);
}

auto RenderDisplay::DrawThreeByteColumns(services::Area const area, services::Image const image) -> bool {
  return DrawColumnImage(area, image, 3);
}

// mode & 7FFF (& 3FFF with three bytes): 0 copy, 1 XOR, 2 AND, 3 OR, 4 transparent copy.
auto RenderDisplay::DrawColumnImage(services::Area const area, services::Image const image, std::size_t const planes) -> bool {
  auto const operation{ image.mode & (planes == 4 ? 0x7FFF : 0x3FFF) };
  if (!Inside(area) || operation > 4) return false;
  if (operation == 0) {
    CopyColumnImage(area, image, planes);
  } else if (operation == 4) {
    _frame.Draw(*Required(_textures.Fetch(ColumnImage(area, image.bytes, planes), Paint(Keying::COLOUR_ZERO))), ColumnPoint({ .x = area.x, .y = area.y }));
  } else {
    CombineColumnImage(area, image, planes);
  }
  return true;
}

// A copy: rows twice with mode bit 15; three bytes leave colour bit 3 clear, or set with mode bit 14.
auto RenderDisplay::CopyColumnImage(services::Area const area, services::Image const image, std::size_t const planes) -> void {
  auto paint{ Paint(Keying::OPAQUE) };
  if (planes == 3 && (image.mode & SETS_BIT_3) != 0) paint.colours = WithBit3(paint.colours);
  auto       destination{ ColumnRect(area) };
  auto const doubled{ (image.mode & DOUBLED_ROWS) != 0 };
  if (doubled) destination.height *= 2;
  _frame.Draw(*Required(_textures.Fetch(ColumnImage(area, image.bytes, planes), paint)), destination);
}

// XOR, AND or OR with the screen's colour numbers; three bytes keep colour bit 3 under AND.
auto RenderDisplay::CombineColumnImage(services::Area const area, services::Image const image, std::size_t const planes) -> void {
  auto const operation{ image.mode & 0x03 };
  auto const kept{ planes == 3 ? 0x08 : 0x00 };
  auto const rect{ ColumnRect(area) };
  auto const over{ Decoded(ColumnImage(area, image.bytes, planes)) };
  auto const under{ _frame.Numbers(rect) };
  auto const combined{ Reworked(under, over, [operation, kept](std::uint8_t const screen, std::uint8_t const value) {
    if (operation == 1) return static_cast<std::uint8_t>(screen ^ value);
    if (operation == 2) return static_cast<std::uint8_t>(screen & (value | kept));
    return static_cast<std::uint8_t>(screen | value);
  }) };
  _frame.Put(combined, Point{ .x = rect.x, .y = rect.y });
}

// Low byte of mode the colour; (mode & 7FFF) >> 8: 0 opaque (0 bits colour 0), 4 set, 1-3 the pen, others the
// previous pen. Mode bit 15 doubles rows.
auto RenderDisplay::DrawBits(services::Area const area, services::Image const image) -> bool {
  auto const code{ static_cast<std::uint8_t>((image.mode & 0x7FFF) >> 8) };
  if (!Inside(area)) return false;
  auto const bits{ BitsImage(area.width, area.height, image.bytes) };
  auto       destination{ ColumnRect(area) };
  if ((image.mode & DOUBLED_ROWS) != 0) destination.height *= 2;
  if (code == 0) {
    _frame.Fill(destination, 0);
    _frame.DrawInk(*Shape(bits), destination, static_cast<std::uint8_t>(image.mode));
    return true;
  }
  _pen.Select(code == 4 ? 0 : code, static_cast<std::uint8_t>(image.mode));
  DrawWithPen(bits, destination);
  return true;
}

// 1 bits colour (mode & 15), 0 bits colour ((mode >> 8) & 15).
auto RenderDisplay::DrawBitsOnPaper(services::Area const area, services::Image const image) -> void {
  if (!Inside(area)) return;
  auto const destination{ ColumnRect(area) };
  _frame.Fill(destination, static_cast<std::uint8_t>(image.mode >> 8));
  _frame.DrawInk(*Shape(BitsImage(area.width, area.height, image.bytes)), destination, static_cast<std::uint8_t>(image.mode));
}

// x and width in pixels, whole source bytes drawn (the bits past the width included); high byte of mode: 0 or 4
// set, 1-3 the pen, others the previous pen.
auto RenderDisplay::DrawShiftedBits(services::Area const area, services::Image const image) -> bool {
  if (!InsidePixels(area)) return false;
  auto const code{ static_cast<std::uint8_t>(image.mode >> 8) };
  _pen.Select(code == 4 ? 0 : code, static_cast<std::uint8_t>(image.mode));
  auto const width_bytes{ ((area.width - 1uz) >> 3) + 1 };
  DrawWithPen(BitsImage(width_bytes, area.height, image.bytes), At(FramePoint(area.x, area.y), Size{ .width = static_cast<int>(width_bytes * 8), .height = area.height }));
  return true;
}

// The set bits with the pen: a shape blit for a set pen, the screen's numbers combined for the others.
auto RenderDisplay::DrawWithPen(GameImage const& bits, Rect const destination) -> void {
  if (!_pen.Draws()) return;
  if (_pen.Sets()) {
    _frame.DrawInk(*Shape(bits), destination, _pen.Colour());
    return;
  }
  auto const shape{ RowsRepeated(Decoded(bits), static_cast<std::size_t>(destination.height) / bits.layout.height) };
  auto const drawn{ Reworked(_frame.Numbers(destination), shape,
                             [this](std::uint8_t const number, std::uint8_t const bit) { return bit != 0 ? _pen.UnderSetBit(number) : _pen.UnderClearBit(number); }) };
  _frame.Put(drawn, Point{ .x = destination.x, .y = destination.y });
}

// The set bits as a white shape, the clear ones transparent.
auto RenderDisplay::Shape(GameImage const& bits) -> SharedBaked {
  return Required(_textures.Fetch(bits, render::Paint{ .colours = INK, .keying = Keying::OPAQUE }));
}

// A transparent copy from pixel x (colour 0 keeps the screen), unclipped; no rectangle draws nothing.
auto RenderDisplay::DrawShiftedColumns(services::Area const area, std::span<std::uint8_t const> const image) -> void {
  if (area.width == 0 || area.height == 0) return;
  auto const game_image{ ColumnImage({ .x = 0, .y = 0, .width = area.width, .height = area.height }, image, 4) };
  _frame.Draw(*Required(_textures.Fetch(game_image, Paint(Keying::COLOUR_ZERO))), FramePoint(area.x, area.y));
}

auto RenderDisplay::SaveColumns(services::Area const area, std::span<std::uint8_t> const into) -> void {
  if (area.width == 0 || area.height == 0) return;
  CopyInto(assets::EncodePlanes(_frame.Numbers(ColumnRect(area)), assets::InterleavedLayout(area.width, area.height, 4)), into);
}

auto RenderDisplay::CopyColumns(services::Area const from, services::Point const to) -> void {
  if (from.width == 0 || from.height == 0) return;
  _frame.Copy(ColumnRect(from), ColumnPoint(to));
}

auto RenderDisplay::DecodePicture(services::Point const at, PictureByteReader const& reader) -> bool {
  return DrawPicture(at, reader, assets::Layout::STREAM_8);
}

auto RenderDisplay::DecodeMaskedPicture(services::Point const at, PictureByteReader const& reader) -> bool {
  return DrawPicture(at, reader, assets::Layout::STREAM_16);
}

// The stream as it draws from pixel x & ~3 of line y, then the pack's picture over it when the stream is a shipped one.
auto RenderDisplay::DrawPicture(services::Point const at, PictureByteReader const& reader, assets::Layout const stream) -> bool {
  auto const room{ RoomAt(at) };
  auto const colours{ stream == assets::Layout::STREAM_8 ? assets::Colours::EIGHT : assets::Colours::SIXTEEN };
  auto const picture{ assets::DecodePictureAsDrawn(reader, colours, room) };
  if (!picture) return false;
  auto const origin{ FramePoint(at.x & ~3, at.y) };
  _frame.Draw(*Required(_textures.Fetch(*picture, Paint(Keying::OPAQUE))), origin);
  if (auto const shipped{ _textures.FetchShipped(*picture, stream, Paint(Keying::OPAQUE)) }) _frame.Draw(**shipped, origin);
  return true;
}

// ---- shapes --------------------------------------------------------------------------------------------------------

auto RenderDisplay::Box(services::Area const area, Word const colour) -> bool {
  if (!Inside(area)) return false;
  _frame.Fill(ColumnRect(area), static_cast<std::uint8_t>(colour));
  return true;
}

// XOR (high byte 1), AND (2: no change) or OR (3) the colour over a box, selecting that pen.
auto RenderDisplay::Highlight(services::Area const area, Word const colour) -> bool {
  auto const code{ static_cast<std::uint8_t>(colour >> 8) };
  if (!Inside(area) || code == 0 || code >= 4) return false;
  _pen.Select(code, static_cast<std::uint8_t>(colour));
  if (code == 2) return true;
  auto const rect{ ColumnRect(area) };
  auto const under{ _frame.Numbers(rect) };
  _frame.Put(Reworked(under, under, [this](std::uint8_t const number, std::uint8_t) { return _pen.UnderSetBit(number); }), Point{ .x = rect.x, .y = rect.y });
  return true;
}

// Points on the game area only; a set line in two colours (its dashes' gaps in colour 0), any other operation on
// the screen's numbers.
auto RenderDisplay::Line(services::Corners const ends, Word const colour) -> void {
  auto const ink{ InkOf(colour) };
  auto const points{ DashedPoints(ends, ink.pattern) };
  if (points.empty()) return;
  if (ink.operation == 0 || ink.operation > 4) {
    auto const placed{ [&points](bool const on) {
      return points | std::views::filter([on](DashedPoint const& point) { return point.on == on; }) | std::views::transform(&DashedPoint::at) | std::ranges::to<std::vector>();
    } };
    _frame.Plot(placed(true), ink.colour);
    _frame.Plot(placed(false), 0);
    return;
  }
  auto const area{ Bounds(points) };
  auto       numbers{ _frame.Numbers(area).Pixels() | std::ranges::to<std::vector>() };
  std::ranges::for_each(points, [&](DashedPoint const& point) {
    auto& number{ numbers.at(static_cast<std::size_t>((point.at.y - area.y) * area.width + point.at.x - area.x)) };
    number = LineApplied(number, ink, point.on);
  });
  _frame.Put(assets::Image{ static_cast<std::size_t>(area.width), static_cast<std::size_t>(area.height), std::move(numbers) }, Point{ .x = area.x, .y = area.y });
}

// Pixels x1..x2, y1..y2 inclusive; corners in the wrong order draw nothing.
auto RenderDisplay::Fill(services::Corners const corners, Word const colour) -> void {
  if (corners.to.x < corners.from.x || corners.to.y < corners.from.y) return;
  auto const size{
    Size{ .width = corners.to.x - corners.from.x + 1, .height = corners.to.y - corners.from.y + 1 }
  };
  _frame.Fill(At(FramePoint(corners.from.x, corners.from.y), size), static_cast<std::uint8_t>(colour));
}

// ---- battle cells --------------------------------------------------------------------------------------------------

auto RenderDisplay::SaveCell(services::Point const at, services::Cell const into) -> void {
  SaveCellAt(ColumnPoint(at), into);
}

auto RenderDisplay::SaveShiftedCell(services::Point const at, services::Cell const into) -> void {
  SaveCellAt(HalfColumnRight(ColumnPoint(at)), into);
}

auto RenderDisplay::RestoreCell(services::Point const at, services::CellImage const cell) -> void {
  RestoreCellAt(ColumnPoint(at), cell);
}

auto RenderDisplay::RestoreShiftedCell(services::Point const at, services::CellImage const cell) -> void {
  RestoreCellAt(HalfColumnRight(ColumnPoint(at)), cell);
}

auto RenderDisplay::SaveCellAt(Point const at, services::Cell const into) -> void {
  CopyInto(assets::EncodePlanes(_frame.Numbers(At(at, CELL_SIZE)), assets::CELL_IMAGE_LAYOUT), into);
}

auto RenderDisplay::RestoreCellAt(Point const at, services::CellImage const cell) -> void {
  auto const image{
    GameImage{ .layout = assets::CELL_IMAGE_LAYOUT, .bytes = BytesOf(cell), .shipped = std::nullopt }
  };
  _frame.Draw(*Required(_textures.Fetch(image, Paint(Keying::OPAQUE))), at);
}

auto RenderDisplay::CopyCellBlock(services::Point const from, services::Point const to) -> void {
  _frame.Copy(At(ColumnPoint(from), BLOCK_SIZE), ColumnPoint(to));
}

// ---- cell buffers in memory: the game's own bytes, composed by the library's routines --------------------------

auto RenderDisplay::OverlayCell(services::Cell const cell, services::CellImage const image) -> void {
  opeu_vram_draw(cell.data(), image.data());
}

auto RenderDisplay::MaskCell(services::Cell const cell, services::CellMask const mask) -> void {
  opeu_vram_load(cell.data(), mask.data());
}

auto RenderDisplay::DrawCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::Point const at) -> void {
  opeu_vram_glyph(cell.data(), glyph.data(), at.x, at.y);
}

auto RenderDisplay::DrawColouredCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::ColouredPlace const place) -> void {
  opeu_vram_glyph_color(cell.data(), glyph.data(), place.at.x, place.at.y, place.colour);
}

// ---- sprites -------------------------------------------------------------------------------------------------------

// The screen rectangle's numbers where the mask keeps them, ORed with the image; the result in the buffer, plane-block
// by plane-block, and on the screen.
auto RenderDisplay::ComposeSprite(services::Area const area, services::Sprite const& sprite) -> void {
  if (area.width == 0 || area.height == 0) return;
  auto const planes{ (sprite.flags & 1) != 0 ? 4uz : 3uz };
  auto const keep{ Decoded(BitsImage(area.width, area.height, sprite.and_mask)) };
  auto const image{ Decoded(ColumnImage(area, sprite.or_image, planes)) };
  auto const rect{ ColumnRect(area) };
  auto const composed{ Compose(rect, keep, image) };
  CopyInto(assets::EncodePlanes(composed, assets::PlaneBlocksLayout(area.width, area.height, 4)), sprite.buffer);
  _frame.Put(composed, Point{ .x = rect.x, .y = rect.y });
}

// x and width in pixels: (width >> 3) + 1 columns from column x >> 3, the mask and image (width >> 3 bytes a row)
// shifted right by x & 7 within them; the screen is written only with flags bit 0.
auto RenderDisplay::ComposeShiftedSprite(services::Area const area, services::Sprite const& sprite) -> void {
  auto const source_bytes{ static_cast<std::size_t>(area.width >> 3) };
  if (source_bytes == 0 || area.height == 0) return;
  auto const columns{ source_bytes + 1 };
  auto const planes{ (sprite.flags & 1) != 0 ? 4uz : 3uz };
  auto const shift{ area.x & 7uz };
  auto const keep{ Shifted(Decoded(BitsImage(source_bytes, area.height, sprite.and_mask)), shift, columns * 8, 1) };
  auto const image{ Shifted(Decoded(ColumnImage({ .x = 0, .y = 0, .width = static_cast<Word>(source_bytes), .height = area.height }, sprite.or_image, planes)), shift, columns * 8, 0) };
  auto const rect{ At(FramePoint(area.x & ~7, area.y), Size{ .width = static_cast<int>(columns * 8), .height = area.height }) };
  auto const composed{ Compose(rect, keep, image) };
  CopyInto(assets::EncodePlanes(composed, assets::PlaneBlocksLayout(columns, area.height, 4)), sprite.buffer);
  if ((sprite.flags & 1) != 0) _frame.Put(composed, Point{ .x = rect.x, .y = rect.y });
}

auto RenderDisplay::Compose(Rect const area, assets::Image const& keep, assets::Image const& image) -> assets::Image {
  auto const kept{ Reworked(_frame.Numbers(area), keep, [](std::uint8_t const number, std::uint8_t const bit) { return bit != 0 ? number : std::uint8_t{ 0 }; }) };
  return Reworked(kept, image, [](std::uint8_t const number, std::uint8_t const value) { return static_cast<std::uint8_t>(number | value); });
}

// ---- the platform's side -------------------------------------------------------------------------------------------

auto RenderDisplay::Present() -> void {
  _frame.Present(_mode == Mode::GRAPHICS ? WHITE : BLACK);
}

auto RenderDisplay::Capture() -> assets::Texture {
  return _frame.Capture(_mode == Mode::GRAPHICS ? WHITE : BLACK);
}

auto RenderDisplay::RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> const frame, std::span<std::uint8_t, PALETTE_ENTRIES * 3> const palette) -> void {
  std::ranges::copy(_palette.EntryLevels(), palette.begin());
  if (_mode == Mode::TEXT) {
    std::ranges::fill(frame, 0);
    return;
  }
  auto const numbers{ _frame.Numbers(LOGICAL_SCREEN) };
  std::ranges::transform(numbers.Pixels(), frame.begin(), [this](std::uint8_t const number) { return _palette.EntryOf(number); });
}

}
