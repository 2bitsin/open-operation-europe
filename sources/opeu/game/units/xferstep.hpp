// The supply transfer's step and its driver.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_transfer_step(Game& game);
uint16_t opeu_transfer_run(Game& game, uint16_t interactive);
