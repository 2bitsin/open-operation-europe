// The daily new-weapon announcement (core/daypres).
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_daily_present(uint8_t* ds, HostEvent event, uint16_t sp);

uint16_t svc_daypres(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
