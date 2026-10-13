#pragma once
// The renderer's coordinates and colours: game pixels on the 640x480 logical screen, colours as 8-bit RGBA.

#include <cstdint>

namespace opeu::render::detail::geometry {

struct Point {
  int x{};
  int y{};

  auto operator==(Point const&) const -> bool = default;
};

struct Size {
  int width{};
  int height{};

  auto operator==(Size const&) const -> bool = default;
};

struct Rect {
  int x{};
  int y{};
  int width{};
  int height{};

  auto operator==(Rect const&) const -> bool = default;
};

struct Rgba {
  std::uint8_t r{};
  std::uint8_t g{};
  std::uint8_t b{};
  std::uint8_t a{ 0xFF };

  auto operator==(Rgba const&) const -> bool = default;
};

// The screen the game draws on; a window shows it scaled.
inline constexpr auto LOGICAL_SIZE{
  Size{ .width = 640, .height = 480 }
};
inline constexpr auto LOGICAL_SCREEN{
  Rect{ .x = 0, .y = 0, .width = LOGICAL_SIZE.width, .height = LOGICAL_SIZE.height }
};

inline constexpr auto BLACK{
  Rgba{ .r = 0x00, .g = 0x00, .b = 0x00 }
};
inline constexpr auto WHITE{
  Rgba{ .r = 0xFF, .g = 0xFF, .b = 0xFF }
};

[[nodiscard]] constexpr auto At(Point const origin, Size const size) noexcept -> Rect {
  return Rect{ .x = origin.x, .y = origin.y, .width = size.width, .height = size.height };
}

[[nodiscard]] constexpr auto Dimensions(Rect const rect) noexcept -> Size {
  return Size{ .width = rect.width, .height = rect.height };
}

// A colour scaled by `level` / 255 per channel, alpha kept: a fade's modulation at that level.
[[nodiscard]] constexpr auto Dimmed(Rgba const colour, std::uint8_t const level) noexcept -> Rgba {
  auto const scale{ [level](std::uint8_t const channel) { return static_cast<std::uint8_t>(channel * level / 0xFF); } };
  return Rgba{ .r = scale(colour.r), .g = scale(colour.g), .b = scale(colour.b), .a = colour.a };
}

}

namespace opeu::render {
using detail::geometry::At;
using detail::geometry::BLACK;
using detail::geometry::Dimensions;
using detail::geometry::Dimmed;
using detail::geometry::LOGICAL_SCREEN;
using detail::geometry::LOGICAL_SIZE;
using detail::geometry::Point;
using detail::geometry::Rect;
using detail::geometry::Rgba;
using detail::geometry::Size;
using detail::geometry::WHITE;
}
