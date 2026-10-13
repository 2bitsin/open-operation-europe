// Delivering a regiment request: the regiment is built and, for a person's side, shown arriving.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_request_action2(Game& game, PendingEvent event, uint8_t side, uint16_t request);

// The services opeu_request_action2 calls.
uint16_t svc_reqspawn(Game& game, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d);
