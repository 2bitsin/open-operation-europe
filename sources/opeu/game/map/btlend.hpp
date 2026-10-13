#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_battle_run(Game& game);
void opeu_battle_refresh_strengths(Game& game);
void opeu_battle_init_element(Game& game, uint16_t element, uint8_t side, uint8_t index);
void opeu_battle_init_elements(Game& game);
void opeu_battle_objective_damage(Game& game);
void opeu_notice_other(Game& game);
