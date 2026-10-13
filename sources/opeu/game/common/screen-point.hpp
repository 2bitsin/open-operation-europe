// A point on the screen: x in pixels or in 8-pixel byte columns (the planar screen's unit; each user says which), y in
// pixel rows. The one screen position type of the game: the mouse, the map's cell pixels and the data segment's
// print places.
#pragma once

#include <cstdint>

namespace opeu::game::detail::screen_point {

struct ScreenPoint {
  std::uint16_t x;
  std::uint16_t y;

  friend constexpr auto operator==(ScreenPoint, ScreenPoint) -> bool = default;
};
} // namespace opeu::game::detail::screen_point

namespace opeu::game {
using detail::screen_point::ScreenPoint;
} // namespace opeu::game
