// The air strike: its hourly countdown, the flight and the bombs.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_air_damage_roll(Game& game, uint8_t x, uint8_t y, uint8_t n);
void     opeu_air_destroyed_message(Game& game, uint16_t what);
uint16_t opeu_air_strike_due(Game& game);
void     opeu_air_strike_present(Game& game);
void     opeu_air_strike_hit(Game& game, uint8_t x, uint8_t y, uint16_t n);
void     opeu_air_strike_event(Game& game);
