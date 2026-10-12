#pragma once

#include "types.hpp"

uint16_t opeu_arrival_locate(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t), uint16_t x, uint16_t y, uint8_t side);

// The services of this module.
uint16_t svc_arrplace(uint8_t* ds, uint16_t id, uint16_t side, uint16_t off, uint16_t seg, uint16_t unused);
