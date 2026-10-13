// Distributing allocated equipment over units and objectives.
#pragma once

#include "common/events.hpp"
#include "types.hpp"
#include "units/allocation.hpp"

void opeu_alloc_distribute(Game& game, opeu::game::Allocation const& allocation);
void opeu_alloc_choose_hq(Game& game);
void opeu_alloc_finish(Game& game);
