// A side's request reaching headquarters: accepted and booked with its delay, or turned down.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_pending_start_present(Game& game);
void opeu_request_start(Game& game, uint8_t side, uint16_t request);
void opeu_pending_fallback_present(Game& game);
void opeu_request_fallback(Game& game, uint8_t side, uint16_t request);
