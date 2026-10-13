// The battalion-type table of the arms request.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_rq_draw_types(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags);

uint16_t svc_rqsuptbl(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
