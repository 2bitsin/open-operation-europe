// The hourly handling of the sides' requests: a booked request is carried out when its delay runs out, and each hour a
// filed request may reach headquarters, which accepts or turns it down.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_hourly_c_stage_b(uint8_t* ds, PendingEvent event,
                           uint16_t stale_threshold);

// The services opeu_hourly_c_stage_b calls.
uint16_t svc_requests(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b,
                      uint16_t c, uint16_t d);
