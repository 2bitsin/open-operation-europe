// The computer side's passes after its turn: staff, division leaders, restocking from objectives, requests,
// reinforcements, air plans and the attack target.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_ai_equip_element(Game& game, uint16_t element, uint8_t kind, uint8_t side);
uint16_t opeu_ai_raise_unit(Game& game);
void     opeu_ai_reinforce(Game& game);
uint16_t opeu_ai_restock_unit(Game& game, uint16_t objective, uint16_t unit);
uint16_t opeu_ai_restock_from_objectives(Game& game);
void     opeu_ai_mark_group_leaders(Game& game);
void     opeu_ai_15c0(Game& game);
void     opeu_control_other_finish(Game& game);
