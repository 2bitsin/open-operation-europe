#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_exit_to_map(Game& game, uint16_t element, uint8_t x, uint8_t y);
void opeu_battle_element_withdraw(Game& game, uint16_t element);
void opeu_battle_withdraw_step(Game& game, uint16_t element);
void opeu_battle_hold_step(Game& game, uint16_t element);
void opeu_battle_element_step(Game& game, uint16_t element);
