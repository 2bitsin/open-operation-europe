#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_move_element(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint8_t mode);
void     opeu_battle_remove_unit(Game& game, BattleEvent event, uint16_t unit, uint16_t element);
void     opeu_battle_remove_list(Game& game, BattleEvent event, uint16_t unit);
uint8_t  opeu_battle_attack_rating(Game& game, BattleEvent event, uint16_t attacker, uint16_t target);
uint16_t opeu_battle_best_target(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_battle_can_act(Game& game, BattleEvent event, uint16_t element);
void     opeu_battle_update_element(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlmove(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
