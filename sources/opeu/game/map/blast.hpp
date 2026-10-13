#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_location_blast(Game& game, uint8_t x, uint8_t y, uint8_t side);
void opeu_blast_erase(Game& game, uint8_t x, uint8_t y);
void opeu_blast_pause(Game& game);
