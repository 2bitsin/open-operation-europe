#include "opeu/render/source-layer.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <numeric>
#include <ranges>
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
      .first = { .x = CellBelow(area.x),              .y = CellBelow(area.y)               },
        .last = { .x = CellAbove(area.x + area.width), .y = CellAbove(area.y + area.height) }
  });
}

// The frame's cells `area` wholly covers.
[[nodiscard]] constexpr auto Covered(Rect const area) noexcept -> CellRange {
  if (Empty(area)) return {};
  return OnTheFrame({
      .first = { .x = CellAbove(area.x),              .y = CellAbove(area.y)               },
        .last = { .x = CellBelow(area.x + area.width), .y = CellBelow(area.y + area.height) }
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

[[nodiscard]] constexpr auto Corner(Point const cell) noexcept -> Point {
  return Point{ .x = cell.x * CELL, .y = cell.y * CELL };
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
  ++_write;
  auto const covered{ Covered(area) };
  std::ranges::for_each(CellsOf(Touched(area)), [&](Point const cell) {
    if (!Contains(covered, cell)) return Write(cell, Source{});
    auto const corner{ Corner(cell) };
    Write(cell, Source{
                    .image = placed.image, .offset = { .x = corner.x - placed.at.x, .y = corner.y - placed.at.y },
                         .colours = placed.colours
    });
  });
}

auto SourceLayer::Clear(Rect const area) -> void {
  ++_write;
  std::ranges::for_each(CellsOf(Touched(area)), [this](Point const cell) { Write(cell, Source{}); });
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
  auto const whole{ shift.x % CELL == 0 && shift.y % CELL == 0 };
  auto const from{ Touched(kept.area) };
  auto const destination{ geometry::At(at, Dimensions(kept.area)) };
  auto const covered{ Covered(destination) };
  std::ranges::for_each(CellsOf(Touched(destination)), [&](Point const cell) {
    auto const index{ whole && Contains(covered, cell) ? KeptIndex(from, { .x = cell.x - (shift.x / CELL), .y = cell.y - (shift.y / CELL) }) : std::nullopt };
    Write(cell, index ? kept.cells[*index] : Source{});
  });
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
