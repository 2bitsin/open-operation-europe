// The small value types the machine interfaces speak in: places and ranges in game coordinates.
#pragma once

#include <cstdint>

namespace opeu::services::detail::machine_types {

// A place: a pixel, an 8-pixel column and line, or a screen position, as the method taking it says.
struct Point {
  std::uint16_t x{};
  std::uint16_t y{};

  auto operator==(Point const&) const -> bool = default;
};

// An inclusive range of coordinates.
struct Range {
  std::uint16_t min{};
  std::uint16_t max{};

  auto operator==(Range const&) const -> bool = default;
};

}

namespace opeu::services {
using detail::machine_types::Point;
using detail::machine_types::Range;
}
