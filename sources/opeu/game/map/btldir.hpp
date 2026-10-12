#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint8_t opeu_battle_cell_facing(uint8_t* ds, BattleEvent event, uint8_t x,
                                uint8_t y, uint8_t to_x, uint8_t to_y);
uint8_t opeu_battle_element_facing(uint8_t* ds, BattleEvent event,
                                   uint16_t element, uint16_t other);
uint8_t opeu_battle_terrain_mode(uint8_t* ds, BattleEvent event,
                                 uint16_t element, uint16_t other);

// The services of this module.
uint16_t svc_btldir(uint8_t* ds, uint32_t address, const uint16_t* args,
                    unsigned int count);
