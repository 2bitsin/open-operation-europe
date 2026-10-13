// The start of a battle: the attack notice, the engaged units, the command decision and the loss records.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_notice_slot(uint8_t* ds, uint16_t objective, uint8_t slot);
uint16_t opeu_notice_garrison(uint8_t* ds, HostEvent event);
void     opeu_notice_collect(uint8_t* ds, HostEvent event);
void     opeu_notice_records_build(uint8_t* ds, HostEvent event);
uint16_t opeu_notice_can_command(uint8_t* ds, HostEvent event, uint8_t list, uint8_t side);
void     opeu_notice_decide(uint8_t* ds, HostEvent event);
uint16_t opeu_notice_view_battle(uint8_t* ds, HostEvent event);
void     opeu_notice_view_finish(uint8_t* ds, HostEvent event);
void     opeu_notice_attack(uint8_t* ds, HostEvent event);
void     opeu_notice_begin(uint8_t* ds, HostEvent event);

// The services the module calls through its event.
uint16_t svc_notice(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
