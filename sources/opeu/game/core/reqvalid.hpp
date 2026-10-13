// Whether a filed request can still be carried out when its day comes.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_request_valid(Game& game, PendingEvent event, uint8_t side, uint16_t request);

// The services opeu_request_valid calls (and opeu_arrival_locate, which it hands the same callback).
uint16_t svc_reqvalid(Game& game, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d);
