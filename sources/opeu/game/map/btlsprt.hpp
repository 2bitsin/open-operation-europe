#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_battle_sprite_offset(uint8_t* ds, BattleEvent event,
                                   uint8_t sprite);
void opeu_battle_draw_sprite(uint8_t* ds, BattleEvent event, uint16_t offset);
void opeu_battle_draw_extra(uint8_t* ds, BattleEvent event, uint8_t x,
                            uint8_t y, uint8_t code);
void opeu_battle_draw_terrain(uint8_t* ds, BattleEvent event, uint8_t x,
                              uint8_t y, uint8_t code);
uint16_t opeu_battle_sprite_valid(uint8_t* ds, BattleEvent event,
                                  uint8_t sprite);
void     opeu_battle_draw_neighbors(uint8_t* ds, BattleEvent event, uint8_t x,
                                    uint8_t y);

// The services of this module.
uint16_t svc_btlsprt(uint8_t* ds, uint32_t address, const uint16_t* args,
                     unsigned int count);
