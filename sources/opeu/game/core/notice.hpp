// The start of a battle: the attack notice, the engaged units, the command decision and the loss records.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_notice_slot(Game& game, uint16_t objective, uint8_t slot);
uint16_t opeu_notice_garrison(Game& game, HostEvent event);
void     opeu_notice_collect(Game& game, HostEvent event);
void     opeu_notice_records_build(Game& game, HostEvent event);
uint16_t opeu_notice_can_command(Game& game, HostEvent event, uint8_t list, uint8_t side);
void     opeu_notice_decide(Game& game, HostEvent event);
uint16_t opeu_notice_view_battle(Game& game, HostEvent event);
void     opeu_notice_view_finish(Game& game, HostEvent event);
void     opeu_notice_attack(Game& game, HostEvent event);
void     opeu_notice_begin(Game& game, HostEvent event);

// The services the module calls through its event.
uint16_t svc_notice(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
