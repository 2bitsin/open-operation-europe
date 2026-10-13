#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_deploy_facing(Game& game, BattleEvent event, uint16_t element);
void     opeu_deploy_put(Game& game, BattleEvent event, uint16_t element);
void     opeu_deploy_lift(Game& game, BattleEvent event, uint16_t element);
uint16_t opeu_deploy_can_place(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
void     opeu_deploy_show_zone(Game& game, BattleEvent event, uint16_t element);
void     opeu_deploy_hide_zone(Game& game, BattleEvent event, uint16_t element);
void     opeu_deploy_draw_cursor(Game& game, BattleEvent event);
uint16_t opeu_deploy_input(Game& game, BattleEvent event);
uint16_t opeu_deploy_manual(Game& game, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_dplcell(Game& game, uint32_t service, const uint16_t* args, unsigned int count);
