#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_pick_hq(uint8_t* ds, BattleEvent event);
void     opeu_battle_auto_hq(uint8_t* ds, BattleEvent event);
void     opeu_battle_officer_hit(uint8_t* ds, BattleEvent event, uint16_t record, uint16_t text, uint16_t killed);
void     opeu_battle_officer_losses(uint8_t* ds, BattleEvent event);
uint16_t opeu_battle_side_commands(const uint8_t* ds);
void     opeu_battle_hq_shaken(uint8_t* ds, BattleEvent event, uint16_t unit);
void     opeu_battle_hq_lost(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_hqloss(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
