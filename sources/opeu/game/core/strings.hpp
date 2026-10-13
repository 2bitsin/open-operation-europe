// Strings in the data segment, addressed by near pointer.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_string_copy(Game& game, uint16_t destination, uint16_t source);
uint16_t opeu_string_append(Game& game, uint16_t destination, uint16_t source);
uint16_t opeu_string_length(Game& game, uint16_t string);
