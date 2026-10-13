// Row-draw callbacks of a list table (see the .cpp).
#pragma once

#include "common/draw-request.hpp"
#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_units(Game& game, opeu::game::DrawRequest const& request);
void opeu_tbl_draw_orders(Game& game, opeu::game::DrawRequest const& request);
