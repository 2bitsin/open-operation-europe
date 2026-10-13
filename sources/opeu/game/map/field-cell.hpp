// A cell of the 14 x 14 battle field, as the battle routines hand one back.
#pragma once

#include <cstdint>

namespace opeu::game::detail::field_cell {

struct FieldCell {
  std::uint8_t x;
  std::uint8_t y;
};
} // namespace opeu::game::detail::field_cell

namespace opeu::game {
using detail::field_cell::FieldCell;
} // namespace opeu::game
