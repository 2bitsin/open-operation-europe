// The equipment allocation screen.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_alloc_amounts_show(Game& game, uint8_t count);
void opeu_alloc_take(Game& game, uint16_t entry, uint8_t slot, uint16_t amount, uint8_t count);
void opeu_alloc_screen(Game& game, uint8_t first, uint8_t second, uint8_t third, uint8_t count);
