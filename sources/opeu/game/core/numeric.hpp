// Arithmetic helpers: scaled products and capped additions and subtractions on bytes and words.
#pragma once

#include "types.hpp"

uint16_t opeu_mul_div(uint16_t a, uint16_t b, uint16_t denominator);
uint16_t opeu_percent(uint16_t a, uint16_t b);
auto     AddByte(uint8_t value, uint16_t amount, uint16_t limit) -> uint8_t;
auto     SubByte(uint8_t value, uint16_t amount) -> uint8_t;
auto     AddWord(uint16_t value, uint16_t amount, uint16_t limit) -> uint16_t;
auto     SubWord(uint16_t value, uint16_t amount) -> uint16_t;
