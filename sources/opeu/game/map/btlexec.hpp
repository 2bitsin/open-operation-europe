#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_order_command(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t command, uint16_t frame);

// The services of this module.
uint16_t svc_btlexec(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
