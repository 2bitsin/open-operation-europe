#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_deploy_template_cell(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element, uint8_t index);
void     opeu_deploy_by_template(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_by_scan(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_at_random(Game& game, BattleEvent event, uint16_t x, uint16_t y, uint16_t element);
void     opeu_deploy_put_at(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
void     opeu_deploy_garrison(Game& game, BattleEvent event, uint16_t unit);
void     opeu_deploy_auto(Game& game, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_battle_deploy(Game& game, uint32_t address, const uint16_t* args, unsigned count);
uint16_t svc_dplauto(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
