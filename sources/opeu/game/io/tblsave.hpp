// The row-draw and choice callbacks of the saved-game list.
#pragma once

#include "common/draw-request.hpp"
#include "common/events.hpp"

#include <cstdint>

auto opeu_tbl_save_accept() -> std::uint8_t;
void opeu_tbl_draw_saves(Game& game, opeu::game::DrawRequest const& request);
