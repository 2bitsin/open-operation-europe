#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_announce_ready(Game& game, BattleEvent event);

// The services of this module.
uint16_t svc_btlready(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
