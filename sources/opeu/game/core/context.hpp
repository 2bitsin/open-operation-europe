// The commander panel of the daily display and the scenario's turn context.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_daily_context(Game& game, uint8_t side);
void opeu_turn_pre_context(Game& game);
