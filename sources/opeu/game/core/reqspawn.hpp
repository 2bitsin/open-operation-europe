// Delivering a regiment request: the regiment is built and, for a person's side, shown arriving.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_request_action2(uint8_t* ds, PendingEvent event, uint8_t side,
                          uint16_t request);

// The services opeu_request_action2 calls.
uint16_t svc_reqspawn(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                      uint16_t c, uint16_t d);
