#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_deploy_template_cell(uint8_t* ds, BattleEvent event, uint16_t x,
                                   uint16_t y, uint16_t element, uint8_t index);
void     opeu_deploy_by_template(uint8_t* ds, BattleEvent event, uint16_t x,
                                 uint16_t y, uint16_t element);
void opeu_deploy_by_scan(uint8_t* ds, BattleEvent event, uint16_t x, uint16_t y,
                         uint16_t element);
void opeu_deploy_at_random(uint8_t* ds, BattleEvent event, uint16_t x,
                           uint16_t y, uint16_t element);
void opeu_deploy_put_at(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y,
                        uint16_t element);
void opeu_deploy_garrison(uint8_t* ds, BattleEvent event, uint16_t unit);
void opeu_deploy_auto(uint8_t* ds, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_battle_deploy(uint8_t* ds, uint32_t address, const uint16_t* args,
                           unsigned count);
uint16_t svc_dplauto(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
