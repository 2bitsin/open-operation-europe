// The division and mission tables' row-draw callbacks and the service table of every row-draw callback.
#pragma once

#include "common/draw-request.hpp"
#include "common/events.hpp"
#include "types.hpp"

void opeu_tbl_draw_groups(Game& game, opeu::game::DrawRequest const& request);
void opeu_tbl_draw_missions(Game& game, opeu::game::DrawRequest const& request);
