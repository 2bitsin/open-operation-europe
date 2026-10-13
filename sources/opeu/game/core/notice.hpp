// The start of a battle: the attack notice, the engaged units, the command decision and the loss records.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_notice_slot(Game& game, uint16_t objective, uint8_t slot);
uint16_t opeu_notice_garrison(Game& game);
void     opeu_notice_collect(Game& game);
void     opeu_notice_records_build(Game& game);
uint16_t opeu_notice_can_command(Game& game, uint8_t list, uint8_t side);
void     opeu_notice_decide(Game& game);
uint16_t opeu_notice_view_battle(Game& game);
void     opeu_notice_view_finish(Game& game);
void     opeu_notice_attack(Game& game);
void     opeu_notice_begin(Game& game);
