#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_battle_pick_hq(Game& game, BattleEvent event);
void     opeu_battle_auto_hq(Game& game, BattleEvent event);
void     opeu_battle_officer_hit(Game& game, BattleEvent event, uint16_t record, uint16_t text, uint16_t killed);
void     opeu_battle_officer_losses(Game& game, BattleEvent event);
uint16_t opeu_battle_side_commands(Game const& game);
void     opeu_battle_hq_shaken(Game& game, BattleEvent event, uint16_t unit);
void     opeu_battle_hq_lost(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_hqloss(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
