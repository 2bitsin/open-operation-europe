#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint8_t  opeu_deploy_facing(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_deploy_put(uint8_t* ds, BattleEvent event, uint16_t element);
void     opeu_deploy_lift(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_deploy_can_place(uint8_t* ds, BattleEvent event, uint8_t x,
                               uint8_t y, uint16_t element);
void opeu_deploy_show_zone(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_deploy_hide_zone(uint8_t* ds, BattleEvent event, uint16_t element);
void opeu_deploy_draw_cursor(uint8_t* ds, BattleEvent event);
uint16_t opeu_deploy_input(uint8_t* ds, BattleEvent event);
uint16_t opeu_deploy_manual(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_dplcell(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
