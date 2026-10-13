#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "types.hpp"

// A right-aligned unsigned decimal in small upright digits ending at the point.
void opeu_view_number(Game& game, opeu::game::ScreenPoint at, uint16_t value);
void opeu_overview_frame(Game& game);
void opeu_view_frame(Game& game, int16_t x, int16_t y);
