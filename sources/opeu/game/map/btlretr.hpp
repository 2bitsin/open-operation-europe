#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_retreat_cell_ok(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t unit);
void     opeu_battle_retreat_to(Game& game, BattleEvent event, uint16_t unit, uint8_t x, uint8_t y);
void     opeu_battle_unit_retreat(Game& game, BattleEvent event, uint16_t unit, uint8_t side);
void     opeu_battle_morale_check(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlretr(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
