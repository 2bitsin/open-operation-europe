#include "opeu/render/render-display.hpp"

#include "opeu/contract.hpp"
#include "opeu/services/canvas.hpp"
#include "opeu/services/vgalib.hpp"

#include <algorithm>
#include <array>
#include <numeric>
#include <ranges>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace opeu::render::detail::render_display {
namespace {

constexpr auto SCREEN_COLUMNS{ 80 };
constexpr auto GAME_TOP{ 40 };
constexpr auto GAME_LINES{ 400 };
constexpr auto FRAME_WIDTH{ 640 };
constexpr auto STORED_LINES{ 819 }; // the Canvas's store: past it, an offset wraps to the top
constexpr auto CELL_SIZE{
  Size{ .width = 40, .height = 26 }
};
constexpr auto BLOCK_SIZE{
  Size{ .width = 24, .height = 24 }
};

// Mode bits of the column images: rows drawn twice, and colour bit 3 set by a three-byte copy.
constexpr Word DOUBLED_ROWS{ 0x8000 };
constexpr Word SETS_BIT_3{ 0x4000 };
// The pen codes of Highlight's high byte.
constexpr auto XOR_PEN{ 1 };
constexpr auto AND_PEN{ 2 };

// A loop count of the library: 0 runs 65536 times.
[[nodiscard]] constexpr auto Count(Word const count) noexcept -> int {
  return count != 0 ? int{ count } : 0x10000;
}

// The frame pixel of game pixel (x, line).
[[nodiscard]] constexpr auto FramePoint(int const x, int const line) noexcept -> Point {
  return Point{ .x = x, .y = line + GAME_TOP };
}

[[nodiscard]] constexpr auto ColumnPoint(services::Point const at) noexcept -> Point {
  return FramePoint(at.x * 8, at.y);
}

// A column rectangle (x and width in 8-pixel columns, y in game lines) on the frame.
[[nodiscard]] constexpr auto ColumnRect(services::Area const area) noexcept -> Rect {
  return At(ColumnPoint({ .x = area.x, .y = area.y }), Size{ .width = Count(area.width) * 8, .height = Count(area.height) });
}

// The whole bytes a row of `width` pixels takes.
[[nodiscard]] constexpr auto BytesFor(int const width) noexcept -> int {
  return ((width - 1) >> 3) + 1;
}

// A rectangle at pixel x whose rows are whole bytes, and the byte past them a shift reaches.
[[nodiscard]] constexpr auto ShiftedRect(services::Area const area) noexcept -> Rect {
  return At(FramePoint(area.x, area.y), Size{ .width = (BytesFor(Count(area.width)) + 1) * 8, .height = Count(area.height) });
}

// A column rectangle inside the game area and not empty: where the clipped routines draw.
[[nodiscard]] constexpr auto Inside(services::Area const area) noexcept -> bool {
  return area.x < SCREEN_COLUMNS && area.width != 0 && area.x + area.width <= SCREEN_COLUMNS && area.y < GAME_LINES && area.height != 0 && area.y + area.height <= GAME_LINES;
}

// A frame rectangle the frame holds whole.
[[nodiscard]] constexpr auto OnTheFrame(Rect const area) noexcept -> bool {
  return area.x >= 0 && area.y >= 0 && area.width > 0 && area.height > 0 && area.x + area.width <= FRAME_WIDTH && area.y + area.height <= LOGICAL_SIZE.height;
}

// What an unclipped routine may have written for `area`: the rows it spans (its rows run on into the next ones past the
// right edge), or the whole frame when it reaches the end of the store and wraps to the top.
[[nodiscard]] constexpr auto Reach(Rect const area) noexcept -> Rect {
  if (area.y + area.height + 1 > STORED_LINES || area.x + area.width > FRAME_WIDTH * 2) return LOGICAL_SCREEN;
  if (area.x + area.width <= FRAME_WIDTH) return area;
  return Rect{ .x = 0, .y = area.y, .width = FRAME_WIDTH, .height = area.height + 1 };
}

// The smallest frame rectangle holding both ends.
[[nodiscard]] constexpr auto Bounds(services::Corners const ends) noexcept -> Rect {
  auto const left{ std::min(int{ ends.from.x }, int{ ends.to.x }) };
  auto const right{ std::max(int{ ends.from.x }, int{ ends.to.x }) };
  auto const top{ std::min(int{ ends.from.y }, int{ ends.to.y }) };
  auto const bottom{ std::max(int{ ends.from.y }, int{ ends.to.y }) };
  return At(FramePoint(left, top), Size{ .width = right - left + 1, .height = bottom - top + 1 });
}

[[nodiscard]] auto ColumnShape(assets::Layout const layout, services::Area const area) -> assets::RawShape {
  return assets::RawShape{ .layout = layout, .width_bytes = area.width, .height = area.height };
}

// Whether every bit of the mask's rows `rows` in byte column `column` is 0: the image alone shows there.
[[nodiscard]] auto WhollyCut(std::span<std::uint8_t const> const mask, std::size_t const width_bytes, Rect const rows) -> bool {
  return std::ranges::all_of(std::views::iota(rows.y, rows.y + rows.height), [&](int const row) {
    auto const at{ (static_cast<std::size_t>(row) * width_bytes) + static_cast<std::size_t>(rows.x) };
    if (at >= mask.size()) return false;
    return mask[at] == 0;
  });
}

using Colours = std::array<std::array<std::byte, 4>, 256>;

// The colour of each palette entry, RGBA.
[[nodiscard]] auto ColoursOf(std::span<std::uint8_t const, PALETTE_LEVELS> const palette) -> Colours {
  auto colours{ Colours{} };
  std::ranges::transform(std::views::iota(0uz, colours.size()), colours.begin(), [palette](std::size_t const entry) {
    auto const level{ [&](std::size_t const channel) { return std::byte{ palette[(entry * 3) + channel] }; } };
    return std::array{ level(0), level(1), level(2), std::byte{ 0xFF } };
  });
  return colours;
}

// The frame in its colours, RGBA.
[[nodiscard]] auto Coloured(std::span<std::uint8_t const> const frame, Colours const& colours) -> assets::Texture {
  auto rgba{ frame | std::views::transform([&colours](std::uint8_t const number) { return colours.at(number); }) | std::views::join | std::ranges::to<std::vector>() };
  return assets::Texture{ 640uz, 480uz, std::move(rgba) };
}

}

RenderDisplay::RenderDisplay(render::Display display, assets::AssetPack const& pack, assets::ContentIndex const& index) : _display{ std::move(display) }, _images{ pack, index } { }

// ---- recognition ----------------------------------------------------------------------------------------------------

auto RenderDisplay::Lay(Handed const& handed) -> void {
  if (!handed.id) {
    _sources.Clear(handed.area);
    return;
  }
  auto const placed{
    Placed{ .image = _sources.Number(*handed.id), .at = { .x = handed.area.x, .y = handed.area.y }, .colours = handed.colours }
  };
  _sources.Place(handed.area, placed);
}

// The image over a cut: laid where it is recognised, then each cell whose cut keeps any pixel of the screen raw.
auto RenderDisplay::LayCut(Handed const& handed, std::span<std::uint8_t const> const cut) -> void {
  Lay(handed);
  auto const area{ handed.area };
  auto const width_bytes{ static_cast<std::size_t>(area.width / 8) };
  auto const first_row{ ((area.y + CELL - 1) / CELL) * CELL };
  auto const columns{ area.width / 8 };
  auto const rows{ std::max((area.y + area.height - first_row) / CELL, 0) };
  std::ranges::for_each(std::views::iota(0, columns * rows), [&](int const n) {
    auto const column{ n % columns };
    auto const top{ first_row + ((n / columns) * CELL) };
    if (!WhollyCut(cut, width_bytes, Rect{ .x = column, .y = top - area.y, .width = 1, .height = CELL })) _sources.Clear(Rect{ .x = area.x + (column * 8), .y = top, .width = CELL, .height = CELL });
  });
}

// ---- the screen ----------------------------------------------------------------------------------------------------

auto RenderDisplay::EnterGraphics() -> void {
  _native.EnterGraphics();
  _sources.ClearAll();
}

auto RenderDisplay::LeaveGraphics() -> void {
  _native.LeaveGraphics();
  _sources.ClearAll();
}

auto RenderDisplay::Clear() -> void {
  _native.Clear();
  _sources.ClearAll();
}

auto RenderDisplay::ShowIdentityPalette() -> void {
  _native.ShowIdentityPalette();
}

auto RenderDisplay::LoadPalette(std::uint8_t const first, std::span<std::uint8_t const> const levels) -> void {
  _native.LoadPalette(first, levels);
}

// ---- images --------------------------------------------------------------------------------------------------------

// A plain copy (mode 0) is the image; any other operation, or rows drawn twice, makes its cells raw.
auto RenderDisplay::DrawColumns(services::Area const area, services::Image const image) -> bool {
  if (!_native.DrawColumns(area, image)) return false;
  auto       drawn{ ColumnRect(area) };
  auto const plain{ (image.mode & 0x7FFF) == 0 };
  if ((image.mode & DOUBLED_ROWS) != 0) drawn.height *= 2;
  Lay(Handed{ .id = plain ? _images.Raw(ColumnShape(assets::Layout::PLANES_4, area), image.bytes) : std::nullopt, .area = drawn, .colours = 0 });
  return true;
}

// A plain copy with colour bit 3 clear is the image.
auto RenderDisplay::DrawThreeByteColumns(services::Area const area, services::Image const image) -> bool {
  if (!_native.DrawThreeByteColumns(area, image)) return false;
  auto       drawn{ ColumnRect(area) };
  auto const plain{ (image.mode & 0x3FFF) == 0 && (image.mode & (SETS_BIT_3 | DOUBLED_ROWS)) == 0 };
  if ((image.mode & DOUBLED_ROWS) != 0) drawn.height *= 2;
  Lay(Handed{ .id = plain ? _images.Raw(ColumnShape(assets::Layout::PLANES_3, area), image.bytes) : std::nullopt, .area = drawn, .colours = 0 });
  return true;
}

// The opaque code (0) draws the shape in its colour on colour 0; a pen leaves the screen under the clear bits.
auto RenderDisplay::DrawBits(services::Area const area, services::Image const image) -> bool {
  if (!_native.DrawBits(area, image)) return false;
  auto       drawn{ ColumnRect(area) };
  auto const opaque{ (image.mode & 0x7F00) == 0 && (image.mode & DOUBLED_ROWS) == 0 };
  auto const id{ opaque ? _images.Raw(ColumnShape(assets::Layout::BITMAP, area), image.bytes) : std::nullopt };
  if ((image.mode & DOUBLED_ROWS) != 0) drawn.height *= 2;
  Lay(Handed{ .id = id, .area = drawn, .colours = static_cast<std::uint8_t>(image.mode & 0x0F) });
  return true;
}

auto RenderDisplay::DrawBitsOnPaper(services::Area const area, services::Image const image) -> void {
  _native.DrawBitsOnPaper(area, image);
  if (!Inside(area)) return;
  Lay(Handed{
      .id = _images.Raw(ColumnShape(assets::Layout::BITMAP, area), image.bytes), .area = ColumnRect(area), .colours = static_cast<std::uint8_t>((image.mode & 0x0F) | ((image.mode >> 4) & 0xF0)) });
}

auto RenderDisplay::DrawShiftedBits(services::Area const area, services::Image const image) -> bool {
  if (!_native.DrawShiftedBits(area, image)) return false;
  _sources.Clear(ShiftedRect(area));
  return true;
}

auto RenderDisplay::DrawShiftedColumns(services::Area const area, std::span<std::uint8_t const> const image) -> void {
  _native.DrawShiftedColumns(area, image);
  auto const columns{ Count(area.width) + 1 };
  _sources.Clear(Reach(At(FramePoint(area.x & ~7, area.y), Size{ .width = columns * 8, .height = Count(area.height) })));
}

auto RenderDisplay::SaveColumns(services::Area const area, std::span<std::uint8_t> const into) -> void {
  _native.SaveColumns(area, into);
}

// A copy moves the cells it moves whole; on and off the frame's edges it makes what it reaches raw.
auto RenderDisplay::CopyColumns(services::Area const from, services::Point const to) -> void {
  _native.CopyColumns(from, to);
  auto const source{ ColumnRect(from) };
  auto const destination{ At(ColumnPoint(to), Dimensions(source)) };
  if (!OnTheFrame(source) || !OnTheFrame(destination)) {
    _sources.Clear(Reach(destination));
    return;
  }
  _sources.Move(source, ColumnPoint(to));
}

auto RenderDisplay::DecodePicture(services::Point const at, PictureByteReader const& reader) -> bool {
  auto       stream{ std::vector<std::uint8_t>{} };
  auto const drawn{ _native.DecodePicture(at, [&reader, &stream] { return stream.emplace_back(reader()); }) };
  if (drawn) LayPicture(at, stream, assets::Layout::STREAM_8);
  return drawn;
}

auto RenderDisplay::DecodeMaskedPicture(services::Point const at, PictureByteReader const& reader) -> bool {
  auto       stream{ std::vector<std::uint8_t>{} };
  auto const drawn{ _native.DecodeMaskedPicture(at, [&reader, &stream] { return stream.emplace_back(reader()); }) };
  if (drawn) LayPicture(at, stream, assets::Layout::STREAM_16);
  return drawn;
}

// A picture draws from pixel x & ~3 in whole 4-pixel units: the pixels past its width are the stream's too, raw.
auto RenderDisplay::LayPicture(services::Point const at, std::span<std::uint8_t const> const stream, assets::Layout const layout) -> void {
  Expects(stream.size() >= 4, "a drawn picture's header was read");
  auto const width{ stream[0] | (stream[1] << 8) };
  auto const height{ stream[2] | (stream[3] << 8) };
  auto const corner{ FramePoint(at.x & ~3, at.y) };
  _sources.Clear(At(corner, Size{ .width = ((width + 3) >> 2) * 4, .height = height }));
  Lay(Handed{ .id = _images.Stream(layout, stream), .area = At(corner, Size{ .width = width, .height = height }), .colours = 0 });
}

// ---- shapes --------------------------------------------------------------------------------------------------------

auto RenderDisplay::Box(services::Area const area, Word const colour) -> bool {
  if (!_native.Box(area, colour)) return false;
  _sources.Clear(ColumnRect(area));
  return true;
}

// An XOR drawn again over the same place puts back what its cells showed; an AND with every bit changes nothing.
auto RenderDisplay::Highlight(services::Area const area, Word const colour) -> bool {
  if (!_native.Highlight(area, colour)) return false;
  auto const pen{ colour >> 8 };
  if (pen == XOR_PEN) ToggleHighlight(area, colour);
  if (pen != XOR_PEN && pen != AND_PEN) _sources.Clear(ColumnRect(area));
  return true;
}

auto RenderDisplay::ToggleHighlight(services::Area const area, Word const colour) -> void {
  auto const same{ _highlighted && _highlighted->colour == colour && _highlighted->area.x == area.x && _highlighted->area.y == area.y && _highlighted->area.width == area.width
                   && _highlighted->area.height == area.height };
  if (same) {
    _sources.Untoggle(_highlighted->toggled);
    _highlighted.reset();
    return;
  }
  _highlighted = Highlighted{ .area = area, .colour = colour, .toggled = _sources.Toggle(ColumnRect(area)) };
}

auto RenderDisplay::Line(services::Corners const ends, Word const colour) -> void {
  _native.Line(ends, colour);
  auto const bounds{ Bounds(ends) };
  _sources.Clear(OnTheFrame(bounds) ? bounds : LOGICAL_SCREEN);
}

// Corners past each other run on through the library's 16-bit counts.
auto RenderDisplay::Fill(services::Corners const corners, Word const colour) -> void {
  _native.Fill(corners, colour);
  auto const ordered{ corners.from.x <= corners.to.x && corners.from.y <= corners.to.y };
  _sources.Clear(ordered ? Reach(Bounds(corners)) : LOGICAL_SCREEN);
}

// ---- battle cells --------------------------------------------------------------------------------------------------

auto RenderDisplay::SaveCell(services::Point const at, services::Cell const into) -> void {
  _native.SaveCell(at, into);
}

auto RenderDisplay::SaveShiftedCell(services::Point const at, services::Cell const into) -> void {
  _native.SaveShiftedCell(at, into);
}

auto RenderDisplay::RestoreCell(services::Point const at, services::CellImage const cell) -> void {
  _native.RestoreCell(at, cell);
  _sources.Clear(Reach(At(ColumnPoint(at), CELL_SIZE)));
}

auto RenderDisplay::RestoreShiftedCell(services::Point const at, services::CellImage const cell) -> void {
  _native.RestoreShiftedCell(at, cell);
  _sources.Clear(Reach(At(ColumnPoint(at), Size{ .width = CELL_SIZE.width + 8, .height = CELL_SIZE.height })));
}

// A 24x24 block moved: a scroll of the battle map, its cells moved with it.
auto RenderDisplay::CopyCellBlock(services::Point const from, services::Point const to) -> void {
  _native.CopyCellBlock(from, to);
  auto const source{ At(ColumnPoint(from), BLOCK_SIZE) };
  auto const destination{ At(ColumnPoint(to), BLOCK_SIZE) };
  if (!OnTheFrame(source) || !OnTheFrame(destination)) {
    _sources.Clear(Reach(destination));
    return;
  }
  _sources.Move(source, ColumnPoint(to));
}

auto RenderDisplay::OverlayCell(services::Cell const cell, services::CellImage const image) -> void {
  _native.OverlayCell(cell, image);
}

auto RenderDisplay::MaskCell(services::Cell const cell, services::CellMask const mask) -> void {
  _native.MaskCell(cell, mask);
}

auto RenderDisplay::DrawCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::Point const at) -> void {
  _native.DrawCellGlyph(cell, glyph, at);
}

auto RenderDisplay::DrawColouredCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::ColouredPlace const place) -> void {
  _native.DrawColouredCellGlyph(cell, glyph, place);
}

auto RenderDisplay::ComposeSprite(services::Area const area, services::Sprite const& sprite) -> void {
  _native.ComposeSprite(area, sprite);
  _sources.Clear(Reach(ColumnRect(area)));
}

auto RenderDisplay::ComposeShiftedSprite(services::Area const area, services::Sprite const& sprite) -> void {
  _native.ComposeShiftedSprite(area, sprite);
  _sources.Clear(Reach(ShiftedRect(area)));
}

// ---- handles -------------------------------------------------------------------------------------------------------

auto RenderDisplay::Save(services::Area const area) -> services::SnapshotId {
  auto const snapshot{ _native.Save(area) };
  auto const kept{ _kept.Keep(_sources.Keep(ColumnRect(area))) };
  Ensures(kept == snapshot, "the sources are kept under the snapshot's id");
  return snapshot;
}

auto RenderDisplay::Restore(services::SnapshotId const snapshot, services::Point const at) -> void {
  _native.Restore(snapshot, at);
  auto const kept{ _kept.Find(snapshot) };
  if (!kept) return;
  auto const& sources{ kept->get() };
  auto const  area{ At(ColumnPoint(at), Dimensions(sources.area)) };
  if (!OnTheFrame(area) || area.y + area.height > GAME_TOP + GAME_LINES) return;
  _sources.Put(sources, ColumnPoint(at));
}

auto RenderDisplay::TakeCell(services::Point const at) -> services::CellId {
  return _native.TakeCell(at);
}

auto RenderDisplay::TakeShiftedCell(services::Point const at) -> services::CellId {
  return _native.TakeShiftedCell(at);
}

auto RenderDisplay::MaskInCell(services::CellId const cell, services::CellMask const mask) -> void {
  _native.MaskInCell(cell, mask);
}

auto RenderDisplay::OverlayInCell(services::CellId const cell, services::CellImage const image) -> void {
  _native.OverlayInCell(cell, image);
}

auto RenderDisplay::GlyphInCell(services::CellId const cell, services::CellGlyph const glyph, services::Point const at) -> void {
  _native.GlyphInCell(cell, glyph, at);
}

auto RenderDisplay::ColouredGlyphInCell(services::CellId const cell, services::CellGlyph const glyph, services::ColouredPlace const place) -> void {
  _native.ColouredGlyphInCell(cell, glyph, place);
}

auto RenderDisplay::PutCell(services::Point const at, services::CellId const cell) -> void {
  _native.PutCell(at, cell);
  _sources.Clear(Reach(At(ColumnPoint(at), CELL_SIZE)));
}

auto RenderDisplay::PutShiftedCell(services::Point const at, services::CellId const cell) -> void {
  _native.PutShiftedCell(at, cell);
  _sources.Clear(Reach(At(ColumnPoint(at), Size{ .width = CELL_SIZE.width + 8, .height = CELL_SIZE.height })));
}

// The mask's 0 bits clear, the image's bits are ORed: where the mask cuts a whole cell, the image alone shows there.
auto RenderDisplay::DrawSprite(services::Area const area, services::SpriteImage const& sprite) -> void {
  _native.DrawSprite(area, sprite);
  auto const drawn{ Reach(ColumnRect(area)) };
  auto const layout{ (sprite.flags & 1) != 0 ? assets::Layout::PLANES_4 : assets::Layout::PLANES_3 };
  auto const id{ OnTheFrame(drawn) ? _images.Raw(ColumnShape(layout, area), sprite.or_image) : std::nullopt };
  if (!id) {
    _sources.Clear(drawn);
    return;
  }
  LayCut(Handed{ .id = id, .area = drawn, .colours = 0 }, sprite.and_mask);
}

auto RenderDisplay::DrawShiftedSprite(services::Area const area, services::SpriteImage const& sprite) -> void {
  _native.DrawShiftedSprite(area, sprite);
  _sources.Clear(Reach(ShiftedRect(area)));
}

// The cut's 0 bits clear, the ink's 1 bits are ORed in its colour: where the cut is whole, the ink on colour 0.
auto RenderDisplay::DrawStamp(services::Area const area, services::Stamp const& stamp) -> void {
  _native.DrawStamp(area, stamp);
  if (!Inside(area)) return;
  auto const id{ stamp.ink.empty() ? std::nullopt : _images.Raw(ColumnShape(assets::Layout::BITMAP, area), stamp.ink) };
  if (!id) {
    _sources.Clear(ColumnRect(area));
    return;
  }
  LayCut(Handed{ .id = id, .area = ColumnRect(area), .colours = static_cast<std::uint8_t>(stamp.colour & 0x0F) }, stamp.cut);
}

auto RenderDisplay::DrawShiftedStamp(services::Area const area, services::Stamp const& stamp) -> void {
  _native.DrawShiftedStamp(area, stamp);
  _sources.Clear(Reach(ShiftedRect(area)));
}

// ---- overlays: the game's XOR, drawn twice ---------------------------------------------------------------------------

namespace {

// The frame pixels an overlay's shape covers.
[[nodiscard]] auto Covered(services::Overlay const& overlay) -> Rect {
  return std::visit(
      [](auto const& shape) {
        using Shape = std::remove_cvref_t<decltype(shape)>;
        if constexpr (std::is_same_v<Shape, services::Bar>) return Reach(ColumnRect(shape.columns));
        if constexpr (std::is_same_v<Shape, services::Mark>) return Reach(ShiftedRect(shape.pixels));
        if constexpr (std::is_same_v<Shape, services::Trace>) return OnTheFrame(Bounds(shape.ends)) ? Bounds(shape.ends) : LOGICAL_SCREEN;
      },
      overlay.shape);
}

}

auto RenderDisplay::ShowOverlay(services::Overlay const& overlay) -> services::OverlayId {
  auto const shown{ _native.ShowOverlay(overlay) };
  auto const kept{ _overlays.Keep(_sources.Toggle(Covered(overlay))) };
  Ensures(kept == shown, "the sources are kept under the overlay's id");
  return shown;
}

auto RenderDisplay::HideOverlay(services::OverlayId const overlay) -> void {
  _native.HideOverlay(overlay);
  if (auto const toggled{ _overlays.Release(overlay) }) _sources.Untoggle(*toggled);
}

// ---- the platform's side -------------------------------------------------------------------------------------------

// The window and the headless PNG take the frame as palette entries with the pointer, skipped when nothing changed;
// the numbers are coloured only when Capture asks for them.
auto RenderDisplay::Present() -> void {
  _native.Present();
}

auto RenderDisplay::Capture() const -> assets::Texture {
  auto frame{ std::vector<std::uint8_t>(FRAME_PIXELS) };
  auto palette{ std::array<std::uint8_t, PALETTE_LEVELS>{} };
  services::Screen().Render(std::span<std::uint8_t, FRAME_PIXELS>{ frame.data(), FRAME_PIXELS }, palette);
  return Coloured(frame, ColoursOf(palette));
}

auto RenderDisplay::RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> const frame, std::span<std::uint8_t, PALETTE_LEVELS> const palette) -> bool {
  return VgalibRender(frame, palette);
}

auto RenderDisplay::Footprint() const noexcept -> std::size_t {
  auto const sources{ [](KeptSources const& kept) { return kept.cells.capacity() * sizeof(Source); } };
  auto const kept{ std::ranges::fold_left(_kept.Values() | std::views::transform(sources), 0uz, std::plus<>{}) };
  auto const toggled{ std::ranges::fold_left(_overlays.Values() | std::views::transform([&sources](Toggled const& t) { return sources(t.before); }), 0uz, std::plus<>{}) };
  return _sources.Footprint() + kept + toggled;
}

}
