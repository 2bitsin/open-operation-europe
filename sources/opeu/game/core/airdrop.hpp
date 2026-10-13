// The Paratroops command (core/airdrop).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_air_mark_regiment(Game& game);
uint8_t  opeu_air_assign_troops(Game& game);
uint8_t  opeu_air_mark_battalion(Game& game);
uint8_t  opeu_air_paratroop_type(Game& game);
uint16_t opeu_air_choose_forces(Game& game);
uint8_t  opeu_air_drop_delay(Game& game, uint8_t hours);
uint16_t opeu_air_drop_order(Game& game, uint8_t x, uint8_t y, uint8_t hours);
void     opeu_air_drop_command(Game& game);
