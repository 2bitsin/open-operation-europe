#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t opeu_battle_cell_facing(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y);
uint8_t opeu_battle_element_facing(Game& game, BattleEvent event, uint16_t element, uint16_t other);
uint8_t opeu_battle_terrain_mode(Game& game, BattleEvent event, uint16_t element, uint16_t other);

// The services of this module.
uint16_t svc_btldir(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
