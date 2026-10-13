// The general-list callbacks of the staff and division assignment screen.
#pragma once

#include "common/draw-request.hpp"
#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_general_assign_choice(Game& game, uint8_t choice);
void    opeu_general_list_draw_rows(Game& game, opeu::game::DrawRequest const& request);
