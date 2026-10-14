#include "opeu/render/frame-steps.hpp"

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <utility>

namespace opeu::render::detail::frame_steps {
namespace {

[[nodiscard]] auto SizeOf(assets::DrawnPicture const& picture) -> Size {
  return Size{ .width = static_cast<int>(picture.image.Width()), .height = static_cast<int>(picture.image.Height()) };
}

[[nodiscard]] auto Touches(Placed const& placed, Rect const area) -> bool {
  return !Empty(Intersect(Bounds(placed), area));
}

[[nodiscard]] constexpr auto Moved(Rect const rect, Point const by) noexcept -> Rect {
  return Rect{ .x = rect.x + by.x, .y = rect.y + by.y, .width = rect.width, .height = rect.height };
}

// The pixels of a step's own covering, when it has one.
[[nodiscard]] auto CoveredBy(Step const& step) -> std::optional<Rect> {
  return std::visit(
      [](auto const& drawn) -> std::optional<Rect> {
        using Drawn = std::remove_cvref_t<decltype(drawn)>;
        if constexpr (std::is_same_v<Drawn, FillStep>) return drawn.area;
        if constexpr (std::is_same_v<Drawn, ImageStep>) {
          if (drawn.keying == Keying::OPAQUE) return At(drawn.at, drawn.image->Dimensions());
        }
        if constexpr (std::is_same_v<Drawn, PictureStep>) return At(drawn.at, SizeOf(*drawn.picture));
        return std::nullopt;
      },
      step);
}

}

Bits::Bits(std::vector<std::byte> bytes, Size const size, std::optional<assets::Layout> const shipped) : _bytes{ std::move(bytes) }, _size{ size }, _shipped{ shipped } { }

auto Bits::Inverted() const -> Bits {
  return Bits{ _bytes | std::views::transform([](std::byte const byte) { return ~byte; }) | std::ranges::to<std::vector>(), _size };
}

auto Bits::Set(int const x, int const y) const -> bool {
  if (x < 0 || y < 0 || x >= _size.width || y >= _size.height) return false;
  auto const byte{ _bytes.at(static_cast<std::size_t>(y) * WidthBytes() + static_cast<std::size_t>(x) / 8) };
  return (std::to_integer<unsigned>(byte) & (0x80U >> (x % 8))) != 0;
}

auto Bits::Holds(Bits const& other, Point const offset) const -> bool {
  auto const size{ other.Dimensions() };
  return std::ranges::all_of(assets::Grid(static_cast<std::size_t>(size.height), static_cast<std::size_t>(size.width)), [&](auto const place) {
    auto const x{ static_cast<int>(place.second) };
    auto const y{ static_cast<int>(place.first) };
    return !other.Set(x, y) || Set(x + offset.x, y + offset.y);
  });
}

auto Bits::Image() const -> GameImage {
  return GameImage{ .layout = assets::InterleavedLayout(WidthBytes(), static_cast<std::size_t>(_size.height), 1), .bytes = _bytes, .shipped = _shipped };
}

auto KeptImage::Image() const -> GameImage {
  return GameImage{ .layout = layout, .bytes = bytes, .shipped = shipped, .repeat = repeat };
}

auto Intersect(Rect const first, Rect const second) noexcept -> Rect {
  auto const left{ std::max(first.x, second.x) };
  auto const top{ std::max(first.y, second.y) };
  auto const right{ std::min(first.x + first.width, second.x + second.width) };
  auto const bottom{ std::min(first.y + first.height, second.y + second.height) };
  return Rect{ .x = left, .y = top, .width = std::max(right - left, 0), .height = std::max(bottom - top, 0) };
}

auto Empty(Rect const rect) noexcept -> bool {
  return rect.width <= 0 || rect.height <= 0;
}

auto Contains(Rect const outer, Rect const inner) noexcept -> bool {
  return inner.x >= outer.x && inner.y >= outer.y && inner.x + inner.width <= outer.x + outer.width && inner.y + inner.height <= outer.y + outer.height;
}

auto Bounds(Step const& step) -> Rect {
  return std::visit(
      [](auto const& drawn) -> Rect {
        using Drawn = std::remove_cvref_t<decltype(drawn)>;
        if constexpr (std::is_same_v<Drawn, FillStep>) return drawn.area;
        if constexpr (std::is_same_v<Drawn, PointsStep>) return drawn.bounds;
        if constexpr (std::is_same_v<Drawn, InkStep>) return drawn.destination;
        if constexpr (std::is_same_v<Drawn, ImageStep>) return At(drawn.at, drawn.image->Dimensions());
        if constexpr (std::is_same_v<Drawn, PictureStep>) return At(drawn.at, SizeOf(*drawn.picture));
        if constexpr (std::is_same_v<Drawn, RemapStep>) return drawn.area;
        if constexpr (std::is_same_v<Drawn, CombineStep>) return drawn.area;
      },
      step);
}

auto Bounds(Placed const& placed) -> Rect {
  return Intersect(Moved(Bounds(*placed.step), placed.offset), placed.clip);
}

auto Covered(Placed const& placed) -> std::optional<Rect> {
  auto const covered{ CoveredBy(*placed.step) };
  if (!covered) return std::nullopt;
  return Intersect(Moved(*covered, placed.offset), placed.clip);
}

auto Recipe::Add(Placed placed) -> void {
  if (Empty(Bounds(placed))) return;
  if (auto const covered{ Covered(placed) }) std::erase_if(_steps, [covered](Placed const& earlier) { return Contains(*covered, Bounds(earlier)); });
  _steps.push_back(std::move(placed));
}

auto Recipe::Touching(Rect const area) const -> Recipe {
  auto touching{ Recipe{} };
  touching._steps = _steps | std::views::filter([area](Placed const& placed) { return Touches(placed, area); }) | std::ranges::to<std::vector>();
  return touching;
}

auto Recipe::Copied(Rect const from, Point const to) const -> std::vector<Placed> {
  auto const by{
    Point{ .x = to.x - from.x, .y = to.y - from.y }
  };
  auto const landing{ At(to, Dimensions(from)) };
  return _steps | std::views::transform([&](Placed const& placed) {
           return Placed{
             .step = placed.step, .offset = Point{ .x = placed.offset.x + by.x, .y = placed.offset.y + by.y },
                  .clip = Intersect(Moved(placed.clip, by), landing)
           };
         })
         | std::views::filter([](Placed const& placed) { return !Empty(Bounds(placed)); }) | std::ranges::to<std::vector>();
}

auto Recipe::Find(std::uint32_t const overlay) const -> std::optional<std::size_t> {
  auto const found{ std::ranges::find_if(_steps, [overlay](Placed const& placed) { return std::holds_alternative<RemapStep>(*placed.step) && std::get<RemapStep>(*placed.step).overlay == overlay; }) };
  if (found == _steps.end()) return std::nullopt;
  return static_cast<std::size_t>(std::distance(_steps.begin(), found));
}

auto Recipe::Remove(std::size_t const at) -> void {
  _steps.erase(_steps.begin() + static_cast<std::ptrdiff_t>(at));
}

}
