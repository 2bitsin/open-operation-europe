// Order handlers (see the .cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_order_attack_allowed(Game& game, uint8_t x, uint8_t y);
void     opeu_order_attack_present(Game& game);
uint16_t opeu_order_attack_start(Game& game, uint16_t name, uint8_t x, uint8_t y);
uint8_t  opeu_order_occupy_loss(Game& game);
void     opeu_order_occupied_present(Game& game);
uint16_t opeu_order_occupy_run(Game& game);
uint8_t  opeu_order_attack_loss(Game& game);
uint16_t opeu_order_attack_run(Game& game);
