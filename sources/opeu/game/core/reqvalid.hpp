// Whether a filed request can still be carried out when its day comes.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_request_valid(uint8_t* ds, PendingEvent event, uint8_t side,
                            uint16_t request);

// The services opeu_request_valid calls (and opeu_arrival_locate, which it hands the same callback).
uint16_t svc_reqvalid(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                      uint16_t c, uint16_t d);
