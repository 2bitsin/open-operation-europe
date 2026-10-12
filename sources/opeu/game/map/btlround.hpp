#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_battle_closing_music(uint8_t* ds, BattleEvent event, uint8_t side);
void opeu_battle_report_outcome(uint8_t* ds, BattleEvent event);
void opeu_battle_report_stalemate(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlround(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
