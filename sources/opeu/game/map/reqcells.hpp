#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_count_request_cells(Game& game);
uint16_t opeu_request_cells(Game& game, uint8_t side, uint16_t (*predicate)(Game&, uint8_t, uint8_t, uint8_t));

// The services of this module.
uint16_t svc_reqcells(uint8_t* a0, uint8_t a1, uint8_t a2, uint8_t a3);
