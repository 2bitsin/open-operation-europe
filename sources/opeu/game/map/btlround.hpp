#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_closing_music(Game& game, BattleEvent event, uint8_t side);
void opeu_battle_report_outcome(Game& game, BattleEvent event);
void opeu_battle_report_stalemate(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlround(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
