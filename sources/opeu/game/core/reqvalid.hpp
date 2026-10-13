// Whether a filed request can still be carried out when its day comes.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_request_valid(Game& game, uint8_t side, uint16_t request);
