// The supply transfer's step and its driver.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_transfer_step(Game& game, HostWordsEvent event);
uint16_t opeu_transfer_run(Game& game, HostWordsEvent event, uint16_t interactive);

uint16_t svc_xferstep(Game& game, uint16_t id, uint16_t* args, uint16_t count);
