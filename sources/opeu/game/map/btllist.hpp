#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_element_side(Game& game, uint16_t element);
uint8_t  opeu_battle_unit_side(Game& game, uint16_t unit);
uint16_t opeu_battle_element_unit(Game& game, uint16_t element);
uint8_t  opeu_battle_element_count(Game& game, uint16_t unit);
