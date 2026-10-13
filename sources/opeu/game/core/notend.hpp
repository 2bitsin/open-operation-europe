// The end of a battle: occupation, morale, leaders' experience, resumed orders and the loss report.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_end_can_occupy(Game& game, uint8_t x, uint8_t y, uint16_t unit);
void     opeu_end_occupy_present(Game& game);
void     opeu_end_occupy(Game& game);
void     opeu_end_morale(Game& game);
void     opeu_end_leaders(Game& game);
void     opeu_end_select(Game& game, uint8_t side);
void     opeu_end_orders(Game& game);
void     opeu_end_nothing(void);
void     opeu_end_losses(Game& game);
void     opeu_notice_end(Game& game);
