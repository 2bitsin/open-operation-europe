// Byte blocks: copy, fill and compare in the data segment and in far segments, and decimal parsing.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_copy_bytes(Game& game, uint16_t source, uint16_t destination, uint16_t size);
void     opeu_copy_far_to_near(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size);
void     opeu_copy_near_to_far(Game& game, uint16_t source, uint8_t* destination_segment, uint16_t destination, uint16_t size);
void     opeu_read_far_bytes(Game& game, uint8_t* source_segment, uint16_t source, uint16_t destination, uint16_t size);
void     opeu_fill_far_bytes(uint8_t* buffer_segment, uint16_t buffer, uint16_t size, uint8_t value);
uint16_t opeu_string_copy_far(Game& game, uint16_t destination, uint8_t* source_segment, uint16_t source);
uint16_t opeu_bytes_equal(Game& game, uint16_t a, uint16_t b, uint16_t size);
void     opeu_fill_bytes(Game& game, uint16_t buffer, uint16_t size, uint8_t value);
uint32_t opeu_parse_decimal(Game& game, uint16_t text);
