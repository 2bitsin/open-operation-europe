#pragma once

#include "types.hpp"

uint16_t opeu_move_supplied(uint8_t* ds);
void     opeu_move_consume(uint8_t* ds);
void     opeu_move_sighted(uint8_t* ds, uint16_t unit, uint16_t other);
