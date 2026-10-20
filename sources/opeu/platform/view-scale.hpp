#pragma once
// The window's size as an integer multiple of the frame: the arithmetic behind the backend's snapping, free of SDL so
// a test runs it headless. Sizes are in device pixels unless they say window coordinates (points on macOS); the pixel
// density converts between them (2 on a Retina display, where a 640x480-point window is 1280x960 pixels: 2x).

#include <optional>

namespace opeu::platform::detail::view_scale {

// What the frame's own size is measured in: device pixels or window coordinates.
struct Size {
  int width{};
  int height{};

  auto operator==(Size const&) const noexcept -> bool = default;
};

// What a window shows the frame on: the frame's size in pixels, the window's pixels per window coordinate and the
// largest multiple of the frame the screen holds.
struct View {
  Size  frame;
  float density{ 1.0f };
  int   largest{ 1 };
};

// The display a window is on: its usable bounds in window coordinates (the menu bar and dock left out) and its pixels
// per window coordinate.
struct Screen {
  Size  usable;
  float density{ 1.0f };
};

// Window coordinates left around a window at its largest scale: the title bar and a little desktop.
inline constexpr auto SCREEN_MARGIN{ 48 };

// The largest multiple of `frame` that fits `room` (both in pixels), at least 1.
[[nodiscard]] auto FittingScale(Size room, Size frame) -> int;
// The view of `frame` on `screen`.
[[nodiscard]] auto ViewOn(Size frame, Screen const& screen) -> View;
// The multiple of the frame nearest to a window of `pixels`: rounded, not floored; between 1 and the view's largest.
[[nodiscard]] auto NearestScale(View const& view, Size pixels) -> int;
// The window size, in window coordinates, that shows the frame at `scale` (rounded up to whole coordinates, so a
// fractional density never leaves the window a pixel short of the multiple).
[[nodiscard]] auto WindowSize(View const& view, int scale) -> Size;
// The window size to set so a window of `pixels` shows the nearest multiple; nothing when it already does (within the
// rounding of one window coordinate), which ends the resize the snap itself causes.
[[nodiscard]] auto Snapped(View const& view, Size pixels) -> std::optional<Size>;

}

namespace opeu::platform {
using detail::view_scale::FittingScale;
using detail::view_scale::NearestScale;
using detail::view_scale::Screen;
using detail::view_scale::SCREEN_MARGIN;
using detail::view_scale::Size;
using detail::view_scale::Snapped;
using detail::view_scale::View;
using detail::view_scale::ViewOn;
using detail::view_scale::WindowSize;
}
