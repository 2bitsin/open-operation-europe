// Strings in the data segment, addressed by near pointer, and ASCII case conversion.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_string_copy(Game& game, uint16_t destination, uint16_t source);
uint16_t opeu_string_append(Game& game, uint16_t destination, uint16_t source);
uint16_t opeu_string_length(Game& game, uint16_t string);
uint8_t  opeu_ascii_upper(uint8_t code);
uint8_t  opeu_ascii_lower(uint8_t code);
