#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

// The arc of a shot between two screen points.
void opeu_battle_draw_line(Game& game, opeu::game::ScreenPoint from, opeu::game::ScreenPoint to);
