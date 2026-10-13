// Sister Angelique's visit to a unit with a general in poor health or veteran battalions.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_angel_unit_eligible(Game& game, uint16_t unit);
uint8_t  opeu_angel_portrait(Game& game, uint8_t side);
void     opeu_angel_finish(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_demoralize(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_inspire(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_judge_side(Game& game, uint16_t person, uint16_t unit);
void     opeu_angel_present(Game& game, uint16_t code_segment);
void     opeu_angel_event(Game& game, uint16_t code_segment);
