// The dated noon events, their actions and the leaders' speeches.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_noon_no_action(Game& game, const uint8_t* far_text);
void     opeu_noon_block_axis_regts(Game& game, const uint8_t* far_text);
void     opeu_noon_block_axis_sup_bns(Game& game, const uint8_t* far_text);
void     opeu_noon_block_axis_bns_regts(Game& game, const uint8_t* far_text);
void     opeu_noon_allow_axis_bns_regts(Game& game, const uint8_t* far_text);
void     opeu_noon_raise_axis_morale(Game& game, const uint8_t* far_text);
void     opeu_noon_lower_axis_morale(Game& game, const uint8_t* far_text);
void     opeu_noon_lower_allied_morale(Game& game, const uint8_t* far_text);
void     opeu_noon_axis_morale_blockade(Game& game, const uint8_t* far_text);
void     opeu_code_0e57_00dc(Game& game, const uint8_t* far_text);
void     opeu_code_0e57_010e(Game& game, const uint8_t* far_text);
void     opeu_code_0e57_0140(Game& game, const uint8_t* far_text);
uint16_t opeu_noon_portrait(Game& game, const uint8_t* far_text, uint16_t person, uint16_t show);
void     opeu_noon_speech(Game& game, const uint8_t* far_text, uint16_t speech, uint16_t person);
void     opeu_noon_leader(Game& game, const uint8_t* far_text, uint8_t code, uint8_t line);
void     opeu_turn_noon_events(Game& game, const uint8_t* far_text, uint16_t code_segment);
