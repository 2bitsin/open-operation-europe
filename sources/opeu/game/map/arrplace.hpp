#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_arrival_locate(Game& game, uint16_t (*event)(Game&, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t), uint16_t x, uint16_t y, uint8_t side);

// The services of this module.
uint16_t svc_arrplace(Game& game, uint16_t id, uint16_t side, uint16_t off, uint16_t seg, uint16_t unused);
