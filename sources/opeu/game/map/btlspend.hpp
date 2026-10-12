#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_battle_spend_fuel(uint8_t* ds, BattleEvent event, uint16_t element,
                            uint16_t before);
void opeu_battle_spend_ammo(uint8_t* ds, BattleEvent event, uint16_t element,
                            uint16_t before);
void opeu_battle_spend_stores(uint8_t* ds, BattleEvent event, uint16_t element,
                              uint16_t before);

// The services of this module.
uint16_t svc_btlspend(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
