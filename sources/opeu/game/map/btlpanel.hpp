#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_element_panel(Game& game, BattleEvent event, uint16_t element, uint16_t orders_hidden);

// The services of this module.
uint16_t svc_btlpanel(Game& game, uint32_t address, const uint16_t* args, unsigned int count);
