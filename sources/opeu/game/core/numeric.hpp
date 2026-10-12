// Arithmetic helpers: scaled products and capped additions and subtractions on bytes and words.
#pragma once

#include "types.hpp"

uint16_t opeu_mul_div(uint16_t a, uint16_t b, uint16_t denominator);
uint16_t opeu_percent(uint16_t a, uint16_t b);
uint8_t  opeu_add_byte(uint16_t amount, uint16_t limit, uint8_t* value);
uint8_t  opeu_sub_byte(uint16_t amount, uint8_t* value);
uint16_t opeu_add_word(uint16_t amount, uint16_t limit, uint16_t* value);
uint16_t opeu_sub_word(uint16_t amount, uint16_t* value);
