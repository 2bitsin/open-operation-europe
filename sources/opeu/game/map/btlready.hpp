#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_battle_announce_ready(uint8_t* ds, BattleEvent event,
                                    uint16_t frame);

// The services of this module.
uint16_t svc_btlready(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count);
