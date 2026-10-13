// The colour a message writes a side's name in.
#pragma once

#include <cstdint>

namespace opeu::game::detail::side_color {

// The colour code of the side (the character after ESC 'C' in a message): side 0 '6', side 1 '5'.
[[nodiscard]] constexpr auto SideColorCode(std::uint8_t side) noexcept -> char {
  return side == 0 ? '6' : '5';
}

// The text colour of the side's names and rows (the TEXT_COLOR service's argument): side 0 6, side 1 5.
[[nodiscard]] constexpr auto SideTextColor(std::uint8_t side) noexcept -> std::uint16_t {
  return side == 0 ? 6 : 5;
}

} // namespace opeu::game::detail::side_color

namespace opeu::game {
using detail::side_color::SideColorCode;
using detail::side_color::SideTextColor;
} // namespace opeu::game
