#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_element_panel(uint8_t* ds, BattleEvent event, uint16_t element, uint16_t orders_hidden,
                               uint16_t frame);

// The services of this module.
uint16_t svc_btlpanel(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count);
