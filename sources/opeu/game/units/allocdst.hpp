// Distributing allocated equipment over units and objectives.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_alloc_distribute(Game& game, uint8_t first, uint8_t second, uint8_t third, uint8_t count);
void opeu_alloc_choose_hq(Game& game);
void opeu_alloc_finish(Game& game);
