#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_battle_side_needs_input(Game& game, uint8_t side);
void     opeu_battle_run_rounds(Game& game);
void     opeu_battle_tire_leaders(Game& game);
void     opeu_battle_start_music(Game& game);
void     opeu_battle_reset_element(Game& game, uint16_t element, uint8_t flag, uint8_t index);
void     opeu_battle_reset_elements(Game& game);
void     opeu_notice_one(Game& game);
