// List windows (ListWindow in the data segment): the frame, scrolling, the selection bar and mouse tracking.
#pragma once

#include "common/events.hpp"
#include "common/table-rows.hpp"

#include <cstdint>
#include <utility>

namespace opeu::game::detail::list_call {

// The column opeu_list_scroll is given to redraw the window from its origin, or at the current position.
inline constexpr std::uint8_t REDRAW_ORIGIN{ 0xff };
inline constexpr std::uint8_t REDRAW_HERE{ 0xfe };

} // namespace opeu::game::detail::list_call

namespace opeu::game {
using detail::list_call::REDRAW_HERE;
using detail::list_call::REDRAW_ORIGIN;
} // namespace opeu::game

void opeu_table_columns(Game& game, std::uint8_t column, std::uint8_t rows, std::uint16_t first, std::uint16_t second, std::uint16_t third);
auto opeu_list_mouse_key(Game& game) -> std::uint8_t;
void opeu_list_invert(Game& game);
void opeu_list_scroll(Game& game, opeu::game::ListWindow const& window, std::uint8_t column, std::uint8_t row);
