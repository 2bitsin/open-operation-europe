// The row-draw and choice callbacks of the saved-game list.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_tbl_save_accept() -> std::uint8_t;
void opeu_tbl_draw_saves(Game& game, std::uint8_t column, std::uint8_t first, std::uint8_t row, std::uint8_t last, std::uint8_t flags);
