#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_random_target(Game& game, uint16_t element);
void     opeu_battle_element_turn(Game& game, uint16_t element);
uint16_t opeu_battle_unit_fight(Game& game, uint16_t unit);
uint16_t opeu_battle_round(Game& game);
uint16_t opeu_battle_rounds(Game& game);
void     opeu_battle_finish_turn(Game& game);
void     opeu_battle_finish_none(Game& game);
