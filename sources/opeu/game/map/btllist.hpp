#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_battle_element_side(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_unit_side(Game& game, BattleEvent event, uint16_t unit);
uint16_t opeu_battle_element_unit(Game& game, BattleEvent event, uint16_t element);
uint8_t  opeu_battle_element_count(Game& game, BattleEvent event, uint16_t unit);

// The services of this module.
uint16_t svc_btllist(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
