#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_battle_side_needs_input(uint8_t* ds, BattleEvent event,
                                      uint8_t side);
void     opeu_battle_run_rounds(uint8_t* ds, BattleEvent event);
void     opeu_battle_tire_leaders(uint8_t* ds, BattleEvent event);
void     opeu_battle_start_music(uint8_t* ds, BattleEvent event);
void opeu_battle_reset_element(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint8_t flag, uint8_t index);
void opeu_battle_reset_elements(uint8_t* ds, BattleEvent event);
void opeu_notice_one(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlsetup(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
