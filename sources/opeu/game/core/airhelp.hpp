// The air force's dialog helpers shared by the bombing, paratroop and airlift commands.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_air_ask_count(Game& game, uint16_t text, uint16_t low, uint16_t high, int cancel, uint16_t person, uint16_t items);
uint16_t opeu_air_free_planes(Game& game);
uint16_t opeu_air_target_valid(Game& game, uint8_t x, uint8_t y, uint16_t what);
uint16_t opeu_air_confirm(Game& game);
