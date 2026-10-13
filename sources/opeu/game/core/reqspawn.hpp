// Delivering a regiment request: the regiment is built and, for a person's side, shown arriving.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_request_action2(Game& game, uint8_t side, uint16_t request);
