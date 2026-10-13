#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_side_needs_input(Game& game, BattleEvent event, uint8_t side);
void     opeu_battle_run_rounds(Game& game, BattleEvent event);
void     opeu_battle_tire_leaders(Game& game, BattleEvent event);
void     opeu_battle_start_music(Game& game, BattleEvent event);
void     opeu_battle_reset_element(Game& game, BattleEvent event, uint16_t element, uint8_t flag, uint8_t index);
void     opeu_battle_reset_elements(Game& game, BattleEvent event);
void     opeu_notice_one(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlsetup(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
