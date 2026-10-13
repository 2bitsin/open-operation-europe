#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_supply_notice(Game& game, uint16_t element, uint8_t supply);
uint16_t opeu_battle_target_in_range(Game& game, uint16_t element);
uint16_t opeu_battle_cell_in_range(Game& game, uint16_t element);
void     opeu_battle_animate_facing(Game& game, uint16_t element, uint8_t mode);
uint8_t  opeu_battle_step_into(Game& game, uint16_t element, uint8_t x, uint8_t y);
uint8_t  opeu_battle_next_step(Game& game, uint16_t element, uint16_t x, uint16_t y);
uint16_t opeu_battle_element_path_step(Game& game, uint16_t element, uint16_t x, uint16_t y);
