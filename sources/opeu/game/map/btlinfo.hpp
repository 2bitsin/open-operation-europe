#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_cursor_info(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlinfo(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count);
