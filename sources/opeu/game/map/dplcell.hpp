#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_deploy_facing(Game& game, uint16_t element);
void     opeu_deploy_put(Game& game, uint16_t element);
void     opeu_deploy_lift(Game& game, uint16_t element);
uint16_t opeu_deploy_can_place(Game& game, uint8_t x, uint8_t y, uint16_t element);
void     opeu_deploy_show_zone(Game& game, uint16_t element);
void     opeu_deploy_hide_zone(Game& game, uint16_t element);
uint16_t opeu_deploy_input(Game& game);
uint16_t opeu_deploy_manual(Game& game, uint16_t element);
