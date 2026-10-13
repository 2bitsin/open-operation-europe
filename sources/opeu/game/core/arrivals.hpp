// The arrival of reserve units.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

// event(game, id, a0, a1, a2, a3): the services of the arrivals (svc_arrivals).
void opeu_process_arrivals(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t), uint8_t side);
void opeu_hourly_c_stage_c(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// The services of this module as direct calls.
uint16_t svc_arrivals(Game& game, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3);
