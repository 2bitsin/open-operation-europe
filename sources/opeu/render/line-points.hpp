#pragma once
// The points of a line as the game's library walks them: Bresenham from the end with the smaller major coordinate,
// the error starting at half the major length, the minor step +1 when x and y increase together and -1 otherwise.
// Coordinates are 16-bit words as the game passes them; a point keeps them (a caller drops the ones off its screen).

#include <cstdint>
#include <vector>

namespace opeu::render::detail::line_points {

struct LinePoint {
  std::uint16_t x{};
  std::uint16_t y{};

  auto operator==(LinePoint const&) const -> bool = default;
};

// Every point from one end to the other, in drawing order.
[[nodiscard]] auto LinePoints(LinePoint from, LinePoint to) -> std::vector<LinePoint>;

}

namespace opeu::render {
using detail::line_points::LinePoint;
using detail::line_points::LinePoints;
}
