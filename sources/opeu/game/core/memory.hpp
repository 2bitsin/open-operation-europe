// Byte blocks: copy and fill in the data segment, and copies from far segments.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_copy_bytes(Game& game, uint16_t source, uint16_t destination, uint16_t size);
void     opeu_copy_far_to_near(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size);
void     opeu_read_far_bytes(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size);
uint16_t opeu_string_copy_far(Game& game, uint16_t destination, uint8_t* source_segment, uint16_t source);
void     opeu_fill_bytes(Game& game, uint16_t buffer, uint16_t size, uint8_t value);
