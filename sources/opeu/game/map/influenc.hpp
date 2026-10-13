#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_update_influence_map(Game& game);
void opeu_turn_hourly_c(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t));

// The services of this module.
uint16_t svc_influenc(Game& game, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d);
