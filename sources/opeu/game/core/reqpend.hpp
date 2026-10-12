// A side's request reaching headquarters: accepted and booked with its delay, or turned down.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_pending_start_present(uint8_t* ds, PendingEvent event);
void opeu_request_start(uint8_t* ds, PendingEvent event, uint8_t side,
                        uint16_t request);
void opeu_pending_fallback_present(uint8_t* ds, PendingEvent event);
void opeu_request_fallback(uint8_t* ds, PendingEvent event, uint8_t side,
                           uint16_t request);

// The services the pending-request routines call.
uint16_t svc_reqpend(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                     uint16_t c, uint16_t d);
