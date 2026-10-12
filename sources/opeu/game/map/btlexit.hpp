#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_exit_ok(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t element);
uint16_t opeu_battle_cursor_exit_ok(uint8_t* ds, BattleEvent event, uint16_t element);

// The services of this module.
uint16_t svc_btlexit(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count);
