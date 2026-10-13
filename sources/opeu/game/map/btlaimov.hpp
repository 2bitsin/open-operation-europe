#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

uint8_t opeu_battle_ai_profile(Game& game, uint16_t unit, uint16_t all);
auto    opeu_battle_ai_move_to(Game& game, opeu::game::GridCell cell, uint16_t element) -> void;
void    opeu_battle_ai_attack(Game& game, uint16_t element, uint16_t unit, uint16_t target);
void    opeu_battle_ai_attack_near(Game& game, uint16_t element, uint16_t unit);
void    opeu_battle_ai_attack_mid(Game& game, uint16_t element, uint16_t unit);
void    opeu_battle_ai_attack_far(Game& game, uint16_t element, uint16_t unit);
