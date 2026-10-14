#include "opeu/render/render-display.hpp"

#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/contract.hpp"
#include "opeu/render/line-points.hpp"
#include "opeu/services/syslib-gfx.hpp"

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
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
constexpr auto GLYPH_SIZE{
  Size{ .width = 8, .height = 6 }
};

// Mode bits of the column images: rows drawn twice, and colour bit 3 set by a three-byte copy.
constexpr Word DOUBLED_ROWS{ 0x8000 };
constexpr Word SETS_BIT_3{ 0x4000 };

// What the game's memory holds where a saved rectangle's or a battle cell's bytes were: a mark no image of the game
// begins with, then the handle's id.
constexpr auto HANDLE_MARK{ std::to_array<std::uint8_t>({ 0x4F, 0x50, 0x45, 0x55, 0x2D, 0x68, 0x61, 0x6E, 0x64, 0x6C, 0x65, 0x00 }) };
constexpr auto HANDLE_BYTES{ HANDLE_MARK.size() + 4 };

auto WriteHandle(std::span<std::uint8_t> const into, std::uint32_t const id) -> void {
  Expects(into.size() >= HANDLE_BYTES, "the game's buffer holds a handle");
  std::ranges::copy(HANDLE_MARK, into.begin());
  std::ranges::transform(std::views::iota(0U, 4U), into.begin() + HANDLE_MARK.size(), [id](unsigned const n) { return static_cast<std::uint8_t>(id >> (8 * n)); });
}

[[nodiscard]] auto ReadHandle(std::span<std::uint8_t const> const bytes) -> std::optional<std::uint32_t> {
  if (bytes.size() < HANDLE_BYTES || !std::ranges::equal(bytes.first(HANDLE_MARK.size()), HANDLE_MARK)) return std::nullopt;
  auto const id{ bytes.subspan(HANDLE_MARK.size(), 4) };
  return std::ranges::fold_left(id | std::views::reverse, 0U, [](std::uint32_t const value, std::uint8_t const byte) { return value << 8 | byte; });
}

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

// Half a column right: where a shifted battle cell lies.
[[nodiscard]] constexpr auto HalfColumnRight(Point const at) noexcept -> Point {
  return Point{ .x = at.x + 4, .y = at.y };
}

[[nodiscard]] constexpr auto ColumnRect(services::Area const area) noexcept -> Rect {
  return At(ColumnPoint({ .x = area.x, .y = area.y }), Size{ .width = area.width * 8, .height = area.height });
}

// A rectangle at pixel x, `width_bytes` whole bytes wide.
[[nodiscard]] constexpr auto PixelRect(services::Area const area, std::size_t const width_bytes) noexcept -> Rect {
  return At(FramePoint(area.x, area.y), Size{ .width = static_cast<int>(width_bytes * 8), .height = area.height });
}

// The whole bytes a row of `width` pixels takes.
[[nodiscard]] constexpr auto BytesFor(std::size_t const width) noexcept -> std::size_t {
  return ((width - 1) >> 3) + 1;
}

[[nodiscard]] constexpr auto Origin(Rect const rect) noexcept -> Point {
  return Point{ .x = rect.x, .y = rect.y };
}

// The largest picture that fits from game pixel `at` (none off the game area).
[[nodiscard]] auto RoomAt(services::Point const at) -> assets::Room {
  if (at.x >= FRAME_WIDTH || at.y >= GAME_LINES) return assets::Room{ .width = 0, .height = 0 };
  return assets::Room{ .width = static_cast<std::size_t>(FRAME_WIDTH - at.x), .height = static_cast<std::size_t>(GAME_LINES - at.y) };
}

// `count` of the game's bytes, kept (0 past their end).
[[nodiscard]] auto Copied(std::span<std::uint8_t const> const bytes, std::size_t const count) -> std::vector<std::byte> {
  auto kept{ std::vector<std::byte>(count) };
  std::ranges::transform(bytes | std::views::take(count), kept.begin(), [](std::uint8_t const byte) { return std::byte{ byte }; });
  return kept;
}

[[nodiscard]] auto ImageOf(assets::PlaneLayout const& layout, std::span<std::uint8_t const> const bytes, std::optional<assets::Layout> const shipped) -> SharedImage {
  return std::make_shared<KeptImage const>(KeptImage{ .bytes = Copied(bytes, layout.Bytes()), .layout = layout, .shipped = shipped, .repeat = 1 });
}

// The game's column image of `area` in `planes` bytes per column.
[[nodiscard]] auto ColumnImage(services::Area const area, std::span<std::uint8_t const> const bytes, std::size_t const planes) -> SharedImage {
  return ImageOf(assets::InterleavedLayout(area.width, area.height, planes), bytes, planes == 4 ? assets::Layout::PLANES_4 : assets::Layout::PLANES_3);
}

// A one-bit image of `width_bytes` bytes per row, each row on `repeat` lines.
[[nodiscard]] auto BitsOf(std::size_t const width_bytes, std::size_t const height, std::span<std::uint8_t const> const bytes, std::size_t const repeat = 1) -> SharedBits {
  auto       rows{ Copied(bytes, width_bytes * height) };
  auto const size{
    Size{ .width = static_cast<int>(width_bytes * 8), .height = static_cast<int>(height * repeat) }
  };
  if (repeat == 1) return std::make_shared<Bits const>(std::move(rows), size, assets::Layout::BITMAP);
  auto repeated{ assets::Indices(height * repeat) | std::views::transform([&](std::size_t const line) { return std::span{ rows }.subspan(line / repeat * width_bytes, width_bytes); })
                 | std::views::join | std::ranges::to<std::vector>() };
  return std::make_shared<Bits const>(std::move(repeated), size);
}

[[nodiscard]] auto Decoded(KeptImage const& image) -> assets::Image {
  return Required(assets::DecodePlanes(image.bytes, image.layout).transform_error([](assets::DecodeError const error) { return RenderError{ .detail = std::string{ assets::Describe(error) } }; }));
}

// The steps of a lookup over `area` (under the mask's 1 bits): none when nothing changes, a solid colour when every
// number becomes the same one.
[[nodiscard]] auto Looked(Rect const area, std::optional<SharedBits> const& mask, NumberMap const& map) -> std::vector<Step> {
  if (map == SAME) return {};
  if (Constant(map) && mask)
    return {
      InkStep{ .shape = *mask, .destination = area, .number = map.front() }
    };
  if (Constant(map))
    return {
      FillStep{ .area = area, .number = map.front() }
    };
  return {
    RemapStep{ .area = area, .mask = mask, .map = map, .overlay = 0 }
  };
}

// Whether the last step under `area` cleared to colour 0 every pixel `drawn(x, y)` names (x, y from the area's
// corner): an OR there sets its colour, as the game's mask-then-image sprites rely on.
template <class _Drawn>
[[nodiscard]] auto ClearedUnder(Recipe const& under, Rect const area, _Drawn drawn) -> bool {
  auto const steps{ under.Steps() };
  if (steps.empty() || steps.back().offset != Point{} || !Contains(steps.back().clip, area)) return false;
  auto const& last{ *steps.back().step };
  auto const  pixels{ assets::Grid(static_cast<std::size_t>(area.height), static_cast<std::size_t>(area.width)) };
  auto const  every{ [&](auto const cleared) {
    return std::ranges::all_of(pixels, [&](auto const place) {
      auto const x{ static_cast<int>(place.second) };
      auto const y{ static_cast<int>(place.first) };
      return !drawn(x, y) || cleared(area.x + x, area.y + y);
    });
  } };
  if (std::holds_alternative<InkStep>(last)) {
    auto const& cut{ std::get<InkStep>(last) };
    return cut.number == 0 && every([&cut](int const x, int const y) { return cut.shape->Set(x - cut.destination.x, y - cut.destination.y); });
  }
  if (std::holds_alternative<FillStep>(last)) {
    auto const& fill{ std::get<FillStep>(last) };
    return fill.number == 0 && Contains(fill.area, area);
  }
  return false;
}

// An image combined with what is under it, pixel by pixel: one lookup per colour the image holds, through the mask of
// its pixels of that colour. Over pixels just cleared to 0, an OR (or an XOR) sets the image's colours: the image drawn
// with colour 0 letting them show.
template <class _Lookup>
[[nodiscard]] auto Combined(Recipe const& under, Point const at, SharedImage const& image, _Lookup lookup) -> std::vector<Step> {
  auto const values{ Decoded(*image) };
  auto const area{ At(at, image->Dimensions()) };
  auto const drawn{ [&values](int const x, int const y) { return values.At(static_cast<std::size_t>(x), static_cast<std::size_t>(y)) != 0; } };
  auto const sets{ std::ranges::all_of(SAME, [&lookup](std::uint8_t const v) { return lookup(v).front() == v; }) };
  if (sets && lookup(0) == SAME && ClearedUnder(under, area, drawn))
    return {
      ImageStep{ .image = image, .at = at, .keying = Keying::COLOUR_ZERO, .numbers = SAME }
    };
  auto const colours{ values.Pixels() | std::ranges::to<std::set>() };
  auto       parts{ colours | std::views::filter([&lookup](std::uint8_t const value) { return lookup(value) != SAME; }) | std::views::transform([&](std::uint8_t const value) {
                auto const mask{ Bits::Of(Dimensions(area), [&](int const x, int const y) { return values.At(static_cast<std::size_t>(x), static_cast<std::size_t>(y)) == value; }) };
                return CombineStep::Part{ .mask = std::make_shared<Bits const>(mask), .map = lookup(value) };
              })
              | std::ranges::to<std::vector>() };
  if (parts.empty()) return {};
  return {
    CombineStep{ .area = area, .parts = std::move(parts) }
  };
}

// The colour word of a line: low nibble the colour, bits 4-7 the operation, high byte the dash pattern.
struct LineInk {
  std::uint8_t colour;
  std::uint8_t operation;
  std::uint8_t pattern;
};

[[nodiscard]] constexpr auto InkOf(Word const word) noexcept -> LineInk {
  return LineInk{ .colour = static_cast<std::uint8_t>(word & 0x0F), .operation = static_cast<std::uint8_t>(word >> 4 & 0x0F), .pattern = static_cast<std::uint8_t>(word >> 8) };
}

// The line's points on the game area, in order, the ones its dash pattern draws and the gaps (the pattern advances on
// those points only).
struct Dashes {
  std::vector<Point> on;
  std::vector<Point> off;
};

[[nodiscard]] auto DashesOf(services::Corners const ends, std::uint8_t const pattern) -> Dashes {
  auto const inside{ [](LinePoint const point) { return point.x < FRAME_WIDTH && point.y < GAME_LINES; } };
  auto const points{ LinePoints({ .x = ends.from.x, .y = ends.from.y }, { .x = ends.to.x, .y = ends.to.y }) | std::views::filter(inside) | std::ranges::to<std::vector>() };
  auto       dashes{ Dashes{} };
  std::ranges::for_each(std::views::zip(points, std::views::iota(0uz)), [&](auto const entry) {
    auto const& [point, n]{ entry };
    auto const on{ pattern == 0 || (pattern << (n % 8) & 0x80) != 0 };
    (on ? dashes.on : dashes.off).push_back(FramePoint(point.x, point.y));
  });
  return dashes;
}

// The smallest rectangle holding every point.
[[nodiscard]] auto PointBounds(std::span<Point const> const points) -> Rect {
  auto const [left, right]{ std::ranges::minmax(points | std::views::transform(&Point::x)) };
  auto const [top, bottom]{ std::ranges::minmax(points | std::views::transform(&Point::y)) };
  return Rect{ .x = left, .y = top, .width = right - left + 1, .height = bottom - top + 1 };
}

// The points as a shape over their bounds.
[[nodiscard]] auto ShapeOf(std::span<Point const> const points, Rect const bounds) -> SharedBits {
  auto const placed{ points | std::views::transform([bounds](Point const at) { return std::pair{ at.x - bounds.x, at.y - bounds.y }; }) | std::ranges::to<std::set>() };
  return std::make_shared<Bits const>(Bits::Of(Dimensions(bounds), [&placed](int const x, int const y) { return placed.contains(std::pair{ x, y }); }));
}

[[nodiscard]] auto Exchanging(std::uint8_t const colour) -> NumberMap {
  return Lookup(Operation::XOR, colour);
}

[[nodiscard]] auto Oring(std::uint8_t const colour) -> NumberMap {
  return Lookup(Operation::OR, colour);
}

}

RenderDisplay::RenderDisplay(render::Display display, assets::AssetPack const& pack, assets::ContentIndex const& index) : _frame{ std::move(display), pack, index, _palette.Colours() } { }

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
  _frame.Add(FillStep{ .area = LOGICAL_SCREEN, .number = 0 });
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

auto RenderDisplay::AddAll(std::vector<Step> steps) -> void {
  std::ranges::for_each(steps, [this](Step& step) { _frame.Add(std::move(step)); });
}

// ---- images --------------------------------------------------------------------------------------------------------

auto RenderDisplay::DrawColumns(services::Area const area, services::Image const image) -> bool {
  return DrawColumnImage(area, image, 4);
}

auto RenderDisplay::DrawThreeByteColumns(services::Area const area, services::Image const image) -> bool {
  return DrawColumnImage(area, image, 3);
}

// mode & 7FFF (& 3FFF with three bytes): 0 copy (rows twice with bit 15; three bytes leave colour bit 3 clear, or set
// with bit 14), 1 XOR, 2 AND, 3 OR (three bytes keep colour bit 3 under AND), 4 transparent copy. A copy of a handle's
// bytes puts the saved rectangle back.
auto RenderDisplay::DrawColumnImage(services::Area const area, services::Image const image, std::size_t const planes) -> bool {
  auto const operation{ image.mode & (planes == 4 ? 0x7FFF : 0x3FFF) };
  if (!Inside(area) || operation > 4) return false;
  auto const at{ ColumnPoint({ .x = area.x, .y = area.y }) };
  if (auto const handle{ ReadHandle(image.bytes) }; handle && planes == 4 && operation == 0) {
    Restore(static_cast<services::SnapshotId>(*handle), { .x = area.x, .y = area.y });
    return true;
  }
  auto const kept{ ColumnImage(area, image.bytes, planes) };
  if (operation == 0) {
    auto const repeat{ (image.mode & DOUBLED_ROWS) != 0 ? 2uz : 1uz };
    auto const shown{ std::make_shared<KeptImage const>(KeptImage{ .bytes = kept->bytes, .layout = kept->layout, .shipped = kept->shipped, .repeat = repeat }) };
    auto const bit_3{ planes == 3 && (image.mode & SETS_BIT_3) != 0 };
    _frame.Add(ImageStep{ .image = shown, .at = at, .keying = Keying::OPAQUE, .numbers = bit_3 ? Oring(8) : SAME });
  } else if (operation == 4) {
    _frame.Add(ImageStep{ .image = kept, .at = at, .keying = Keying::COLOUR_ZERO, .numbers = SAME });
  } else {
    auto const combine{ operation == 1 ? Operation::XOR : operation == 2 ? Operation::AND : Operation::OR };
    auto const kept_bits{ static_cast<std::uint8_t>(combine == Operation::AND && planes == 3 ? 8 : 0) };
    AddAll(Combined(_frame.Steps(), at, kept, [combine, kept_bits](std::uint8_t const v) { return Lookup(combine, static_cast<std::uint8_t>(v | kept_bits)); }));
  }
  return true;
}

// Low byte of mode the colour; (mode & 7FFF) >> 8: 0 opaque (0 bits colour 0), 4 set, 1-3 the pen, others the
// previous pen. Mode bit 15 doubles rows.
auto RenderDisplay::DrawBits(services::Area const area, services::Image const image) -> bool {
  auto const code{ static_cast<std::uint8_t>((image.mode & 0x7FFF) >> 8) };
  if (!Inside(area)) return false;
  auto const repeat{ (image.mode & DOUBLED_ROWS) != 0 ? 2uz : 1uz };
  auto const bits{ BitsOf(area.width, area.height, image.bytes, repeat) };
  auto       destination{ ColumnRect(area) };
  destination.height *= static_cast<int>(repeat);
  if (code == 0) {
    AddAll({
        FillStep{ .area = destination, .number = 0 },
        InkStep{ .shape = bits, .destination = destination, .number = static_cast<std::uint8_t>(image.mode & 0x0F) }
    });
    return true;
  }
  _pen.Select(code == 4 ? 0 : code, static_cast<std::uint8_t>(image.mode));
  DrawPen(bits, destination);
  return true;
}

// 1 bits colour (mode & 15), 0 bits colour ((mode >> 8) & 15).
auto RenderDisplay::DrawBitsOnPaper(services::Area const area, services::Image const image) -> void {
  if (!Inside(area)) return;
  auto const destination{ ColumnRect(area) };
  AddAll({
      FillStep{ .area = destination, .number = static_cast<std::uint8_t>(image.mode >> 8 & 0x0F) },
      InkStep{ .shape = BitsOf(area.width, area.height, image.bytes), .destination = destination, .number = static_cast<std::uint8_t>(image.mode & 0x0F) }
  });
}

// x and width in pixels, whole source bytes drawn (the bits past the width included); high byte of mode: 0 or 4
// set, 1-3 the pen, others the previous pen.
auto RenderDisplay::DrawShiftedBits(services::Area const area, services::Image const image) -> bool {
  if (!InsidePixels(area)) return false;
  auto const code{ static_cast<std::uint8_t>(image.mode >> 8) };
  _pen.Select(code == 4 ? 0 : code, static_cast<std::uint8_t>(image.mode));
  auto const width_bytes{ BytesFor(area.width) };
  DrawPen(BitsOf(width_bytes, area.height, image.bytes), PixelRect(area, width_bytes));
  return true;
}

// The set bits with the pen: its colour set, an XOR shown (or undone by the same XOR), an AND's lookup under the clear
// bits, an OR's under the set ones (its colour set where the pixels were just cleared to 0).
auto RenderDisplay::DrawPen(SharedBits const& bits, Rect const destination) -> void {
  switch (_pen.Operation()) {
  case PenOperation::NONE: return;
  case PenOperation::SET:  _frame.Add(InkStep{ .shape = bits, .destination = destination, .number = _pen.Colour() }); return;
  case PenOperation::XOR:  Exchange(RemapStep{ .area = destination, .mask = bits, .map = _pen.UnderSetBit(), .overlay = 0 }); return;
  case PenOperation::AND:  AddAll(Looked(destination, std::make_shared<Bits const>(bits->Inverted()), _pen.UnderClearBit())); return;
  case PenOperation::OR:   break;
  }
  if (ClearedUnder(_frame.Steps(), destination, [&bits](int const x, int const y) { return bits->Set(x, y); })) {
    _frame.Add(InkStep{ .shape = bits, .destination = destination, .number = _pen.Colour() });
    return;
  }
  AddAll(Looked(destination, bits, _pen.UnderSetBit()));
}

// An XOR: the same XOR drawn last over its pixels (XORs aside) is taken back, as the second XOR put the pixels back.
auto RenderDisplay::Exchange(RemapStep step) -> void {
  if (step.map == SAME || _frame.WithdrawLast(step)) return;
  _frame.Add(std::move(step));
}

// A transparent copy from pixel x (colour 0 keeps the screen), unclipped; no rectangle draws nothing.
auto RenderDisplay::DrawShiftedColumns(services::Area const area, std::span<std::uint8_t const> const image) -> void {
  if (area.width == 0 || area.height == 0) return;
  _frame.Add(
      ImageStep{ .image = ColumnImage({ .x = 0, .y = 0, .width = area.width, .height = area.height }, image, 4), .at = FramePoint(area.x, area.y), .keying = Keying::COLOUR_ZERO, .numbers = SAME });
}

auto RenderDisplay::SaveColumns(services::Area const area, std::span<std::uint8_t> const into) -> void {
  if (area.width == 0 || area.height == 0) return;
  WriteHandle(into, std::to_underlying(Save(area)));
}

auto RenderDisplay::CopyColumns(services::Area const from, services::Point const to) -> void {
  if (from.width == 0 || from.height == 0) return;
  auto const source{ ColumnRect(from) };
  _frame.AddCopy(_frame.Under(source), source, ColumnPoint(to));
}

auto RenderDisplay::DecodePicture(services::Point const at, PictureByteReader const& reader) -> bool {
  return DrawPicture(at, reader, assets::Layout::STREAM_8);
}

auto RenderDisplay::DecodeMaskedPicture(services::Point const at, PictureByteReader const& reader) -> bool {
  return DrawPicture(at, reader, assets::Layout::STREAM_16);
}

// The stream as it draws from pixel x & ~3 of line y, then the pack's picture over it when the stream is a shipped one.
auto RenderDisplay::DrawPicture(services::Point const at, PictureByteReader const& reader, assets::Layout const stream) -> bool {
  auto const colours{ stream == assets::Layout::STREAM_8 ? assets::Colours::EIGHT : assets::Colours::SIXTEEN };
  auto       picture{ assets::DecodePictureAsDrawn(reader, colours, RoomAt(at)) };
  if (!picture) return false;
  _frame.Add(PictureStep{ .picture = std::make_shared<assets::DrawnPicture const>(*std::move(picture)), .stream = stream, .at = FramePoint(at.x & ~3, at.y) });
  return true;
}

// ---- shapes --------------------------------------------------------------------------------------------------------

auto RenderDisplay::Box(services::Area const area, Word const colour) -> bool {
  if (!Inside(area)) return false;
  _frame.Add(FillStep{ .area = ColumnRect(area), .number = static_cast<std::uint8_t>(colour & 0x0F) });
  return true;
}

// XOR (high byte 1), AND (2: no change) or OR (3) the colour over a box, selecting that pen.
auto RenderDisplay::Highlight(services::Area const area, Word const colour) -> bool {
  auto const code{ static_cast<std::uint8_t>(colour >> 8) };
  if (!Inside(area) || code == 0 || code >= 4) return false;
  _pen.Select(code, static_cast<std::uint8_t>(colour));
  if (code == 1) Exchange(RemapStep{ .area = ColumnRect(area), .mask = std::nullopt, .map = _pen.UnderSetBit(), .overlay = 0 });
  if (code == 3) AddAll(Looked(ColumnRect(area), std::nullopt, _pen.UnderSetBit()));
  return true;
}

auto RenderDisplay::Line(services::Corners const ends, Word const colour) -> void {
  DrawLine(ends, colour);
}

// Points on the game area only. Set (operation 0, or above 4): the dashes in the colour, the gaps in colour 0. XOR,
// AND, OR: the operation's lookup under the dashes, and under the gaps AND's with colour 0. Operation 4: OR under the
// dashes only.
auto RenderDisplay::DrawLine(services::Corners const ends, Word const colour) -> void {
  auto const ink{ InkOf(colour) };
  auto const dashes{ DashesOf(ends, ink.pattern) };
  auto const plotted{ [this](std::vector<Point> const& points, std::uint8_t const number) {
    if (!points.empty()) _frame.Add(PointsStep{ .points = points, .bounds = PointBounds(points), .number = number });
  } };
  if (ink.operation == 0 || ink.operation > 4) {
    plotted(dashes.on, ink.colour);
    plotted(dashes.off, 0);
    return;
  }
  if (!dashes.on.empty()) {
    auto const bounds{ PointBounds(dashes.on) };
    auto const shape{ ShapeOf(dashes.on, bounds) };
    if (ink.operation == 1) Exchange(RemapStep{ .area = bounds, .mask = shape, .map = Exchanging(ink.colour), .overlay = 0 });
    if (ink.operation == 2) AddAll(Looked(bounds, shape, Lookup(Operation::AND, ink.colour)));
    if (ink.operation >= 3) AddAll(Looked(bounds, shape, Oring(ink.colour)));
  }
  if (ink.operation == 2) plotted(dashes.off, 0);
}

// Pixels x1..x2, y1..y2 inclusive; corners in the wrong order draw nothing.
auto RenderDisplay::Fill(services::Corners const corners, Word const colour) -> void {
  if (corners.to.x < corners.from.x || corners.to.y < corners.from.y) return;
  auto const size{
    Size{ .width = corners.to.x - corners.from.x + 1, .height = corners.to.y - corners.from.y + 1 }
  };
  _frame.Add(FillStep{ .area = At(FramePoint(corners.from.x, corners.from.y), size), .number = static_cast<std::uint8_t>(colour & 0x0F) });
}

// ---- battle cells through the game's memory --------------------------------------------------------------------

auto RenderDisplay::SaveCell(services::Point const at, services::Cell const into) -> void {
  WriteHandle(into, std::to_underlying(TakeCell(at)));
}

auto RenderDisplay::SaveShiftedCell(services::Point const at, services::Cell const into) -> void {
  WriteHandle(into, std::to_underlying(TakeShiftedCell(at)));
}

// A cell's handle puts the cell back; any other bytes are a cell image of the game's own.
auto RenderDisplay::RestoreCell(services::Point const at, services::CellImage const cell) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    PutCell(at, static_cast<services::CellId>(*handle));
    return;
  }
  _frame.Add(ImageStep{ .image = ImageOf(assets::CELL_IMAGE_LAYOUT, cell, std::nullopt), .at = ColumnPoint(at), .keying = Keying::OPAQUE, .numbers = SAME });
}

auto RenderDisplay::RestoreShiftedCell(services::Point const at, services::CellImage const cell) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    PutShiftedCell(at, static_cast<services::CellId>(*handle));
    return;
  }
  _frame.Add(ImageStep{ .image = ImageOf(assets::CELL_IMAGE_LAYOUT, cell, std::nullopt), .at = HalfColumnRight(ColumnPoint(at)), .keying = Keying::OPAQUE, .numbers = SAME });
}

auto RenderDisplay::CopyCellBlock(services::Point const from, services::Point const to) -> void {
  auto const source{ At(ColumnPoint(from), BLOCK_SIZE) };
  _frame.AddCopy(_frame.Under(source), source, ColumnPoint(to));
}

// A cell's handle composes the cell; a buffer of the game's own is composed in its memory by the library's routines.
auto RenderDisplay::OverlayCell(services::Cell const cell, services::CellImage const image) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    OverlayInCell(static_cast<services::CellId>(*handle), image);
    return;
  }
  opeu_vram_draw(cell.data(), image.data());
}

auto RenderDisplay::MaskCell(services::Cell const cell, services::CellMask const mask) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    MaskInCell(static_cast<services::CellId>(*handle), mask);
    return;
  }
  opeu_vram_load(cell.data(), mask.data());
}

auto RenderDisplay::DrawCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::Point const at) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    GlyphInCell(static_cast<services::CellId>(*handle), glyph, at);
    return;
  }
  opeu_vram_glyph(cell.data(), glyph.data(), at.x, at.y);
}

auto RenderDisplay::DrawColouredCellGlyph(services::Cell const cell, services::CellGlyph const glyph, services::ColouredPlace const place) -> void {
  if (auto const handle{ ReadHandle(cell) }) {
    ColouredGlyphInCell(static_cast<services::CellId>(*handle), glyph, place);
    return;
  }
  opeu_vram_glyph_color(cell.data(), glyph.data(), place.at.x, place.at.y, place.colour);
}

// ---- sprites -------------------------------------------------------------------------------------------------------

auto RenderDisplay::ComposeSprite(services::Area const area, services::Sprite const& sprite) -> void {
  DrawSprite(area, { .and_mask = sprite.and_mask, .or_image = sprite.or_image, .flags = sprite.flags });
}

auto RenderDisplay::ComposeShiftedSprite(services::Area const area, services::Sprite const& sprite) -> void {
  DrawShiftedSprite(area, { .and_mask = sprite.and_mask, .or_image = sprite.or_image, .flags = sprite.flags });
}

// ---- handles -------------------------------------------------------------------------------------------------------

auto RenderDisplay::Save(services::Area const area) -> services::SnapshotId {
  auto const rect{ ColumnRect(area) };
  return _kept.Keep(Kept{ .recipe = _frame.Under(rect), .area = rect });
}

auto RenderDisplay::Restore(services::SnapshotId const snapshot, services::Point const at) -> void {
  auto const kept{ _kept.Find(snapshot) };
  if (!kept) return;
  _frame.AddCopy(kept->get().recipe, kept->get().area, ColumnPoint(at));
}

auto RenderDisplay::TakeCell(services::Point const at) -> services::CellId {
  return TakeCellAt(At(ColumnPoint(at), CELL_SIZE));
}

auto RenderDisplay::TakeShiftedCell(services::Point const at) -> services::CellId {
  return TakeCellAt(At(HalfColumnRight(ColumnPoint(at)), CELL_SIZE));
}

auto RenderDisplay::TakeCellAt(Rect const area) -> services::CellId {
  return _cells.Keep(Composed{ .recipe = _frame.Under(area), .area = area });
}

auto RenderDisplay::AddToCell(services::CellId const cell, std::vector<Step> steps) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  std::ranges::for_each(steps, [&composed](Step& step) { composed->get().recipe.Add(Placed{ .step = std::make_shared<Step const>(std::move(step)) }); });
}

// The mask's 0 bits clear the cell to colour 0.
auto RenderDisplay::MaskInCell(services::CellId const cell, services::CellMask const mask) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  auto const cut{ std::make_shared<Bits const>(BitsOf(CELL_SIZE.width / 8, CELL_SIZE.height, mask)->Inverted()) };
  AddToCell(cell, Looked(composed->get().area, cut, Solid(0)));
}

// The image ORed into the cell.
auto RenderDisplay::OverlayInCell(services::CellId const cell, services::CellImage const image) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  AddToCell(cell, Combined(composed->get().recipe, Origin(composed->get().area), ImageOf(assets::CELL_IMAGE_LAYOUT, image, std::nullopt), Oring));
}

// Colour 7 under the glyph's bits (bits 0-2 set, bit 3 cleared).
auto RenderDisplay::GlyphInCell(services::CellId const cell, services::CellGlyph const glyph, services::Point const at) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  auto const area{ composed->get().area };
  AddToCell(cell, {
                      InkStep{ .shape = BitsOf(1, GLYPH_SIZE.height, glyph), .destination = At({ .x = area.x + at.x, .y = area.y + at.y }, GLYPH_SIZE), .number = 7 }
  });
}

// Colour 1 under the glyph's bits for a colour word whose low byte is set, else colour 2.
auto RenderDisplay::ColouredGlyphInCell(services::CellId const cell, services::CellGlyph const glyph, services::ColouredPlace const place) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  auto const area{ composed->get().area };
  auto const number{ static_cast<std::uint8_t>((place.colour & 0xFF) != 0 ? 1 : 2) };
  AddToCell(cell, {
                      InkStep{ .shape = BitsOf(1, GLYPH_SIZE.height, glyph), .destination = At({ .x = area.x + place.at.x, .y = area.y + place.at.y }, GLYPH_SIZE), .number = number }
  });
}

auto RenderDisplay::PutCell(services::Point const at, services::CellId const cell) -> void {
  PutCellAt(ColumnPoint(at), cell);
}

auto RenderDisplay::PutShiftedCell(services::Point const at, services::CellId const cell) -> void {
  PutCellAt(HalfColumnRight(ColumnPoint(at)), cell);
}

auto RenderDisplay::PutCellAt(Point const at, services::CellId const cell) -> void {
  auto const composed{ _cells.Find(cell) };
  if (!composed) return;
  _frame.AddCopy(composed->get().recipe, composed->get().area, at);
}

// The screen cleared under the mask's 0 bits, then the image ORed in (x and width in columns).
auto RenderDisplay::DrawSprite(services::Area const area, services::SpriteImage const& sprite) -> void {
  if (area.width == 0 || area.height == 0) return;
  auto const planes{ (sprite.flags & 1) != 0 ? 4uz : 3uz };
  DrawSpriteAt(ColumnRect(area), ImageOf(assets::InterleavedLayout(area.width, area.height, planes), sprite.or_image, std::nullopt), sprite);
}

// x and width in pixels: the mask and the image (width >> 3 bytes a row) at pixel x, the pixels around them in their
// columns kept; drawn only with flags bit 0.
auto RenderDisplay::DrawShiftedSprite(services::Area const area, services::SpriteImage const& sprite) -> void {
  auto const source_bytes{ static_cast<std::size_t>(area.width >> 3) };
  if (source_bytes == 0 || area.height == 0 || (sprite.flags & 1) == 0) return;
  DrawSpriteAt(PixelRect(area, source_bytes), ImageOf(assets::InterleavedLayout(source_bytes, area.height, 4), sprite.or_image, std::nullopt), sprite);
}

auto RenderDisplay::DrawSpriteAt(Rect const cut, SharedImage const& image, services::SpriteImage const& sprite) -> void {
  auto const keep{ BitsOf(static_cast<std::size_t>(cut.width / 8), static_cast<std::size_t>(cut.height), sprite.and_mask) };
  AddAll(Looked(cut, std::make_shared<Bits const>(keep->Inverted()), Solid(0)));
  AddAll(Combined(_frame.Steps(), Origin(cut), image, Oring));
}

auto RenderDisplay::DrawStamp(services::Area const area, services::Stamp const& stamp) -> void {
  if (!Inside(area)) return;
  DrawStampAt(ColumnRect(area), stamp);
}

auto RenderDisplay::DrawShiftedStamp(services::Area const area, services::Stamp const& stamp) -> void {
  if (!InsidePixels(area)) return;
  DrawStampAt(PixelRect(area, BytesFor(area.width)), stamp);
}

// The cut's 0 bits to colour 0, then the ink's 1 bits in the colour (an OR, which over the cut sets it).
auto RenderDisplay::DrawStampAt(Rect const destination, services::Stamp const& stamp) -> void {
  auto const width_bytes{ static_cast<std::size_t>(destination.width / 8) };
  auto const height{ static_cast<std::size_t>(destination.height) };
  AddAll(Looked(destination, std::make_shared<Bits const>(BitsOf(width_bytes, height, stamp.cut)->Inverted()), Solid(0)));
  if (stamp.ink.empty()) return;
  auto const ink{ BitsOf(width_bytes, height, stamp.ink) };
  if (ClearedUnder(_frame.Steps(), destination, [&ink](int const x, int const y) { return ink->Set(x, y); })) {
    _frame.Add(InkStep{ .shape = ink, .destination = destination, .number = static_cast<std::uint8_t>(stamp.colour & 0x0F) });
    return;
  }
  AddAll(Looked(destination, ink, Oring(stamp.colour)));
}

auto RenderDisplay::NextOverlay() -> std::uint32_t {
  return ++_last_overlay;
}

// The pixels under the overlay's shape through the XOR of its exchanged bits, kept apart in the recipe so that hiding
// it paints them again without it.
auto RenderDisplay::ShowOverlay(services::Overlay const& overlay) -> services::OverlayId {
  auto const id{ NextOverlay() };
  auto const map{ Exchanging(static_cast<std::uint8_t>(overlay.exchange & 0x0F)) };
  auto const shown{ [&](Rect const area, std::optional<SharedBits> mask) { _frame.Add(RemapStep{ .area = area, .mask = std::move(mask), .map = map, .overlay = id }); } };
  std::visit(
      [&](auto const& shape) {
        using Shape = std::remove_cvref_t<decltype(shape)>;
        if constexpr (std::is_same_v<Shape, services::Bar>) shown(ColumnRect(shape.columns), std::nullopt);
        if constexpr (std::is_same_v<Shape, services::Mark>) {
          auto const width_bytes{ BytesFor(shape.pixels.width) };
          shown(PixelRect(shape.pixels, width_bytes), BitsOf(width_bytes, shape.pixels.height, shape.bits));
        }
        if constexpr (std::is_same_v<Shape, services::Trace>) {
          auto const dashes{ DashesOf(shape.ends, shape.dashes) };
          if (dashes.on.empty()) return;
          auto const bounds{ PointBounds(dashes.on) };
          shown(bounds, ShapeOf(dashes.on, bounds));
        }
      },
      overlay.shape);
  return static_cast<services::OverlayId>(id);
}

auto RenderDisplay::HideOverlay(services::OverlayId const overlay) -> void {
  _frame.Withdraw(std::to_underlying(overlay));
}

// ---- the platform's side -------------------------------------------------------------------------------------------

auto RenderDisplay::Present() -> void {
  _frame.Present(_mode == Mode::GRAPHICS ? WHITE : BLACK);
}

auto RenderDisplay::Capture() -> assets::Texture {
  return _frame.Capture(_mode == Mode::GRAPHICS ? WHITE : BLACK);
}

// The frame is written every time (the pump draws its pointer into it); the recipe is painted and read only after a
// change.
auto RenderDisplay::RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> const frame, std::span<std::uint8_t, PALETTE_LEVELS> const palette) -> bool {
  auto const levels{ _palette.EntryLevels() };
  auto const changed{ _frame.Changed() || levels != _shown.levels || _mode != _shown.mode };
  _shown = Shown{ .levels = levels, .mode = _mode };
  std::ranges::copy(levels, palette.begin());
  if (_mode == Mode::TEXT) {
    std::ranges::fill(frame, 0);
    return changed;
  }
  std::ranges::transform(_frame.Numbers().Pixels(), frame.begin(), [this](std::uint8_t const number) { return _palette.EntryOf(number); });
  return changed;
}

}
