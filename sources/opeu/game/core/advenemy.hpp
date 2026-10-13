// The staff's "Enemy Conditions" advice, and the services every advice check calls.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_enemy_mines_or_bombardment(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_division_unfit(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_air_operations(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_attack_plans(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_objective_intent(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_recon_activity(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_plentiful_materials(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_unit_activity(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_commander_location(Game& game, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_need_recon(Game& game, const uint8_t* far_text, HostEvent event);
void     opeu_advice_enemy_conditions(Game& game, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advenemy(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
// Runs the services every advice check calls (AdviceService); `module` names the caller in an unhandled report.
uint16_t svc_advice(const char* module, Game& game, uint16_t id, const uint16_t* args, uint16_t count);
