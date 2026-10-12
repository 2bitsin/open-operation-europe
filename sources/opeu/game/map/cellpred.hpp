#pragma once

#include "types.hpp"

uint16_t opeu_request_cell_allowed(uint8_t* ds, uint8_t x, uint8_t y,
                                   uint8_t side);
uint16_t opeu_arrival_cell_allowed(uint8_t* ds, uint8_t x, uint8_t y,
                                   uint8_t side);
