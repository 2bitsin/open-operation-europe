#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_pick_hq(Game& game);
void     opeu_battle_auto_hq(Game& game);
void     opeu_battle_officer_hit(Game& game, uint16_t record, uint16_t text, uint16_t killed);
void     opeu_battle_officer_losses(Game& game);
uint16_t opeu_battle_side_commands(Game const& game);
void     opeu_battle_hq_shaken(Game& game, uint16_t unit);
void     opeu_battle_hq_lost(Game& game);
