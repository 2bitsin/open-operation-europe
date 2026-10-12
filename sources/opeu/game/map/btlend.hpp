#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_battle_run(uint8_t* ds, BattleEvent event);
void opeu_battle_refresh_strengths(uint8_t* ds, BattleEvent event);
void opeu_battle_init_element(uint8_t* ds, BattleEvent event, uint16_t element,
                              uint8_t side, uint8_t index);
void opeu_battle_init_elements(uint8_t* ds, BattleEvent event);
void opeu_battle_objective_damage(uint8_t* ds, BattleEvent event);
void opeu_notice_other(uint8_t* ds, BattleEvent event);

// The services of this module.
uint16_t svc_btlend(uint8_t* ds, uint32_t address, const uint16_t* args,
                    unsigned int count);
