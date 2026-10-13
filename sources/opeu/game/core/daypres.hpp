// The daily new-weapon announcement (core/daypres).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_daily_present(Game& game, HostEvent event);

uint16_t svc_daypres(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
