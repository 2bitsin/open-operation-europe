#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_battle_mark_exits(uint8_t* ds, BattleEvent event,
                                uint16_t element);
void opeu_battle_unmark_exits(uint8_t* ds, BattleEvent event, uint16_t element);
uint16_t opeu_battle_lone_withdrawal(uint8_t* ds, BattleEvent event,
                                     uint16_t element);

// The services of this module.
uint16_t svc_btlreach(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count);
