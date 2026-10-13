// The staff's "Enemy Conditions" advice, and the services every advice check calls.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_enemy_mines_or_bombardment(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_division_unfit(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_air_operations(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_attack_plans(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_objective_intent(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_recon_activity(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_plentiful_materials(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_unit_activity(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_commander_location(Game& game, const uint8_t* far_text);
uint16_t opeu_advice_enemy_need_recon(Game& game, const uint8_t* far_text);
void     opeu_advice_enemy_conditions(Game& game, const uint8_t* far_text);
