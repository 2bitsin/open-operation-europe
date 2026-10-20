#include "opeu/platform/view-scale.hpp"

#include <algorithm>
#include <cmath>

namespace opeu::platform::detail::view_scale {

namespace {

// Pixels per window coordinate, never below one pixel's worth (a zero or negative density would divide by zero).
[[nodiscard]] auto Density(View const& view) -> float {
  return std::max(view.density, 1.0f / 8.0f);
}

// A window of `pixels` shows the frame at `scale`, within the rounding of one window coordinate.
[[nodiscard]] auto ShowsScale(View const& view, Size const pixels, int const scale) -> bool {
  auto const slack{ static_cast<int>(std::ceil(Density(view))) };
  auto const near{ [slack](int const actual, int const wanted) { return actual >= wanted - slack && actual <= wanted + slack; } };
  return near(pixels.width, view.frame.width * scale) && near(pixels.height, view.frame.height * scale);
}

} // namespace

auto FittingScale(Size const room, Size const frame) -> int {
  if (frame.width <= 0 || frame.height <= 0) return 1;
  return std::max(1, std::min(room.width / frame.width, room.height / frame.height));
}

auto ViewOn(Size const frame, Screen const& screen) -> View {
  auto const pixels{ [&screen](int const coordinates) { return static_cast<int>(std::floor(static_cast<float>(coordinates - SCREEN_MARGIN) * screen.density)); } };
  auto const room{
    Size{ .width = pixels(screen.usable.width), .height = pixels(screen.usable.height) }
  };
  return View{ .frame = frame, .density = screen.density, .largest = FittingScale(room, frame) };
}

auto NearestScale(View const& view, Size const pixels) -> int {
  if (view.frame.width <= 0 || view.frame.height <= 0) return 1;
  auto const across{ static_cast<float>(pixels.width) / static_cast<float>(view.frame.width) };
  auto const down{ static_cast<float>(pixels.height) / static_cast<float>(view.frame.height) };
  return std::clamp(static_cast<int>(std::lround((across + down) / 2.0f)), 1, std::max(1, view.largest));
}

auto WindowSize(View const& view, int const scale) -> Size {
  auto const coordinates{ [&view, scale](int const frame) { return static_cast<int>(std::ceil(static_cast<float>(frame * scale) / Density(view))); } };
  return Size{ .width = coordinates(view.frame.width), .height = coordinates(view.frame.height) };
}

auto Snapped(View const& view, Size const pixels) -> std::optional<Size> {
  auto const scale{ NearestScale(view, pixels) };
  if (ShowsScale(view, pixels, scale)) return std::nullopt;
  return WindowSize(view, scale);
}

}
