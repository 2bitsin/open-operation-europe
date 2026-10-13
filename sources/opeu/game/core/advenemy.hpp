// The staff's "Enemy Conditions" advice, and the services every advice check calls.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_advice_enemy_mines_or_bombardment(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_division_unfit(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_air_operations(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_attack_plans(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_objective_intent(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_recon_activity(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_plentiful_materials(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_unit_activity(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_commander_location(uint8_t* ds, const uint8_t* far_text, HostEvent event);
uint16_t opeu_advice_enemy_need_recon(uint8_t* ds, const uint8_t* far_text, HostEvent event);
void     opeu_advice_enemy_conditions(uint8_t* ds, const uint8_t* far_text, HostEvent event);

// Runs the services of the module's checks.
uint16_t svc_advenemy(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
// Runs the services every advice check calls (AdviceService); `module` names the caller in an unhandled report.
uint16_t svc_advice(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
