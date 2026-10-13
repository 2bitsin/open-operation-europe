#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_path_cost(Game& game, uint8_t x, uint8_t y, uint16_t element);
uint8_t  opeu_battle_path_cost_any(Game& game, uint8_t x, uint8_t y, uint16_t element);
uint16_t opeu_battle_path_step(Game& game, uint16_t x, uint16_t y);
uint16_t opeu_battle_path_flood(Game& game, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint8_t mode);
void     opeu_battle_report_sighted(Game& game, uint16_t element);
uint16_t opeu_battle_spot_chance(Game& game, uint16_t element, uint16_t other);
void     opeu_battle_spot_enemies(Game& game, uint16_t element);
