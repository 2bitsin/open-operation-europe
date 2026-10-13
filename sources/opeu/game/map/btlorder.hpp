#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_battle_execute_order(Game& game, uint16_t element, uint8_t order);
void    opeu_battle_attack_or_hold(Game& game, uint16_t element, uint16_t target);
void    opeu_battle_continue_order(Game& game, uint16_t element);
void    opeu_battle_advance(Game& game, uint16_t element);
void    opeu_battle_seek_target(Game& game, uint16_t element);
