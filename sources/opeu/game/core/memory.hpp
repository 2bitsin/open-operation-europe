// Byte blocks of the data segment copied and filled, and far strings copied into a text.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

#include <cstdint>
#include <span>

void       opeu_copy_bytes(Game& game, uint16_t source, uint16_t destination, uint16_t size);
TextTarget opeu_string_copy_far(Game& game, TextTarget destination, std::span<std::uint8_t const> far_text);
void       opeu_fill_bytes(Game& game, uint16_t buffer, uint16_t size, uint8_t value);
