#include "opeu/render/source-layer.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <ranges>
#include <span>
#include <utility>

namespace opeu::render::detail::source_layer {
namespace {

// The cells from `first` up to, not including, `last`, each way.
struct CellRange {
  Point first;
  Point last;
};

[[nodiscard]] constexpr auto CellBelow(int const pixel) noexcept -> int {
  return pixel >= 0 ? pixel / CELL : -((CELL - 1 - pixel) / CELL);
}

[[nodiscard]] constexpr auto CellAbove(int const pixel) noexcept -> int {
  return -CellBelow(-pixel);
}

[[nodiscard]] constexpr auto OnTheFrame(CellRange const range) noexcept -> CellRange {
  auto const across{ [](int const cell) { return std::clamp(cell, 0, CELL_COLUMNS); } };
  auto const down{ [](int const cell) { return std::clamp(cell, 0, CELL_ROWS); } };
  return CellRange{
    .first = { .x = across(range.first.x), .y = down(range.first.y) },
      .last = { .x = across(range.last.x),  .y = down(range.last.y)  }
  };
}

[[nodiscard]] constexpr auto Empty(Rect const area) noexcept -> bool {
  return area.width <= 0 || area.height <= 0;
}

// The frame's cells `area` touches.
[[nodiscard]] constexpr auto Touched(Rect const area) noexcept -> CellRange {
  if (Empty(area)) return {};
  return OnTheFrame({
      .first = { .x = CellBelow(area.x),              .y = CellBelow(area.y - CELL_TOP)               },
        .last = { .x = CellAbove(area.x + area.width), .y = CellAbove(area.y - CELL_TOP + area.height) }
  });
}

// The frame's cells `area` wholly covers.
[[nodiscard]] constexpr auto Covered(Rect const area) noexcept -> CellRange {
  if (Empty(area)) return {};
  return OnTheFrame({
      .first = { .x = CellAbove(area.x),              .y = CellAbove(area.y - CELL_TOP)               },
        .last = { .x = CellBelow(area.x + area.width), .y = CellBelow(area.y - CELL_TOP + area.height) }
  });
}

[[nodiscard]] constexpr auto Contains(CellRange const range, Point const cell) noexcept -> bool {
  return cell.x >= range.first.x && cell.x < range.last.x && cell.y >= range.first.y && cell.y < range.last.y;
}

[[nodiscard]] constexpr auto Across(CellRange const range) noexcept -> int {
  return std::max(range.last.x - range.first.x, 0);
}

[[nodiscard]] constexpr auto Down(CellRange const range) noexcept -> int {
  return std::max(range.last.y - range.first.y, 0);
}

// The cells of `range`, row by row.
[[nodiscard]] auto CellsOf(CellRange const range) {
  auto const across{ Across(range) };
  return std::views::iota(0, across * Down(range)) | std::views::transform([range, across](int const n) { return Point{ .x = range.first.x + (n % across), .y = range.first.y + (n / across) }; });
}

[[nodiscard]] constexpr auto GridIndex(Point const cell) noexcept -> std::size_t {
  return static_cast<std::size_t>((cell.y * CELL_COLUMNS) + cell.x);
}

// Where `cell` is among the kept cells of `range`; none outside it.
[[nodiscard]] constexpr auto KeptIndex(CellRange const range, Point const cell) noexcept -> std::optional<std::size_t> {
  if (!Contains(range, cell)) return std::nullopt;
  return static_cast<std::size_t>(((cell.y - range.first.y) * Across(range)) + (cell.x - range.first.x));
}

// A non-empty rectangle of whole cells of the grid.
[[nodiscard]] constexpr auto WholeCells(Rect const area) noexcept -> bool {
  auto const top{ area.y - CELL_TOP };
  auto const on_the_grid{ area.x >= 0 && top >= 0 && area.x + area.width <= CELL_COLUMNS * CELL && top + area.height <= CELL_ROWS * CELL };
  return on_the_grid && !Empty(area) && area.x % CELL == 0 && top % CELL == 0 && area.width % CELL == 0 && area.height % CELL == 0;
}

// The grid cell whose top-left corner is frame pixel `at` (a corner of the grid).
[[nodiscard]] constexpr auto CellAt(Point const at) noexcept -> Point {
  return Point{ .x = at.x / CELL, .y = (at.y - CELL_TOP) / CELL };
}

[[nodiscard]] constexpr auto Corner(Point const cell) noexcept -> Point {
  return Point{ .x = cell.x * CELL, .y = (cell.y * CELL) + CELL_TOP };
}

constexpr auto GRID{ static_cast<std::size_t>(CELL_COLUMNS * CELL_ROWS) };

}

SourceLayer::SourceLayer() : _cells(GRID), _writes(GRID) { }

auto SourceLayer::Number(std::string_view const id) -> ImageNumber {
  if (auto const found{ _numbers.find(id) }; found != _numbers.end()) return found->second;
  _names.emplace_back(id);
  auto const number{ static_cast<ImageNumber>(_names.size()) };
  _numbers.emplace(id, number);
  return number;
}

auto SourceLayer::Name(ImageNumber const image) const -> std::optional<std::string_view> {
  auto const n{ static_cast<std::size_t>(std::to_underlying(image)) };
  if (n == 0 || n > _names.size()) return std::nullopt;
  return _names[n - 1];
}

auto SourceLayer::Place(Rect const area, Placed const placed) -> void {
  Clear(area);
  auto const covered{ Covered(area) };
  auto const across{ static_cast<std::size_t>(Across(covered)) };
  std::ranges::for_each(std::views::iota(covered.first.y, covered.last.y), [&](int const row) {
    auto const first{ GridIndex({ .x = covered.first.x, .y = row }) };
    auto const named{ [&](int const column) {
      auto const corner{ Corner({ .x = column, .y = row }) };
      return Source{
        .image = placed.image, .offset = { .x = corner.x - placed.at.x, .y = corner.y - placed.at.y },
             .colours = placed.colours
      };
    } };
    std::ranges::transform(std::views::iota(covered.first.x, covered.last.x), std::span{ _cells }.subspan(first, across).begin(), named);
  });
}

auto SourceLayer::Clear(Rect const area) -> void {
  ++_write;
  auto const range{ Touched(area) };
  auto const across{ static_cast<std::size_t>(Across(range)) };
  std::ranges::for_each(std::views::iota(range.first.y, range.last.y), [&](int const row) {
    auto const first{ GridIndex({ .x = range.first.x, .y = row }) };
    std::ranges::fill(std::span{ _cells }.subspan(first, across), Source{});
    std::ranges::fill(std::span{ _writes }.subspan(first, across), _write);
  });
}

auto SourceLayer::ClearAll() -> void {
  Clear(LOGICAL_SCREEN);
}

auto SourceLayer::Keep(Rect const area) const -> KeptSources {
  return KeptSources{ .area = area, .cells = CellsOf(Touched(area)) | std::views::transform([this](Point const cell) { return At(cell); }) | std::ranges::to<std::vector>() };
}

auto SourceLayer::Put(KeptSources const& kept, Point const at) -> void {
  ++_write;
  auto const shift{
    Point{ .x = at.x - kept.area.x, .y = at.y - kept.area.y }
  };
  auto const destination{ geometry::At(at, Dimensions(kept.area)) };
  if (shift.x % CELL != 0 || shift.y % CELL != 0) return Clear(destination); // no cell moved whole
  auto const from{ Touched(kept.area) };
  auto const covered{ Covered(destination) };
  std::ranges::for_each(CellsOf(Touched(destination)), [&](Point const cell) {
    auto const index{ Contains(covered, cell) ? KeptIndex(from, { .x = cell.x - (shift.x / CELL), .y = cell.y - (shift.y / CELL) }) : std::nullopt };
    Write(cell, index ? kept.cells[*index] : Source{});
  });
}

auto SourceLayer::Move(Rect const from, Point const to) -> void {
  auto const destination{ geometry::At(to, Dimensions(from)) };
  if ((to.x - from.x) % CELL != 0 || (to.y - from.y) % CELL != 0) return Clear(destination); // no cell moved whole
  if (WholeCells(from) && WholeCells(destination)) return MoveCells(from, to);
  _moving.area = from;
  _moving.cells.clear();
  std::ranges::transform(CellsOf(Touched(from)), std::back_inserter(_moving.cells), [this](Point const cell) { return At(cell); });
  Put(_moving, to);
}

// A copy from cell corner to cell corner, on the frame: each row of cells copied as it was (through the scratch, so an
// overlapping copy reads the cells before they are written).
// Row by row in the direction memmove would take, so an overlapping copy reads each cell before it is written.
auto SourceLayer::MoveCells(Rect const from, Point const to) -> void {
  ++_write;
  auto const across{ static_cast<std::size_t>(from.width / CELL) };
  auto const source{ GridIndex(CellAt({ .x = from.x, .y = from.y })) };
  auto const target{ GridIndex(CellAt(to)) };
  auto const rows{ std::views::iota(0uz, static_cast<std::size_t>(from.height / CELL)) };
  auto const row{ [&](std::size_t const n) {
    auto const cells{ std::span{ _cells } };
    auto const at{ n * static_cast<std::size_t>(CELL_COLUMNS) };
    if (target < source)
      std::ranges::copy(cells.subspan(source + at, across), cells.subspan(target + at).begin());
    else
      std::ranges::copy_backward(cells.subspan(source + at, across), cells.subspan(target + at, across).end());
    std::ranges::fill(std::span{ _writes }.subspan(target + at, across), _write);
  } };
  if (target < source)
    std::ranges::for_each(rows, row);
  else
    std::ranges::for_each(rows | std::views::reverse, row);
}

auto SourceLayer::Toggle(Rect const area) -> Toggled {
  auto before{ Keep(area) };
  Clear(area);
  return Toggled{ .before = std::move(before), .write = _write };
}

// Each cell the XOR made raw and nothing wrote since shows again what it showed before.
auto SourceLayer::Untoggle(Toggled const& toggled) -> void {
  ++_write;
  auto const kept{ Touched(toggled.before.area) };
  std::ranges::for_each(CellsOf(kept), [&](Point const cell) {
    if (_writes[GridIndex(cell)] != toggled.write) return;
    Write(cell, toggled.before.cells[KeptIndex(kept, cell).value_or(0)]);
  });
}

auto SourceLayer::At(Point const cell) const -> Source {
  Expects(cell.x >= 0 && cell.x < CELL_COLUMNS && cell.y >= 0 && cell.y < CELL_ROWS, "the cell is on the frame");
  return _cells[GridIndex(cell)];
}

auto SourceLayer::Footprint() const noexcept -> std::size_t {
  return std::transform_reduce(_names.begin(), _names.end(), std::size_t{}, std::plus<>{}, [](std::string const& name) { return name.capacity() + sizeof(std::string) + sizeof(ImageNumber); });
}

auto SourceLayer::Write(Point const cell, Source const source) -> void {
  _cells[GridIndex(cell)]  = source;
  _writes[GridIndex(cell)] = _write;
}

}
