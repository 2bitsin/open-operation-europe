// The Special Forces command: the officer who leads the missions, the pending mission and the mission menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_special_officer(uint8_t* ds, HostEvent event, uint16_t sp);
uint16_t opeu_special_unit_target(uint8_t* ds, HostEvent event, uint16_t sp);
void     opeu_turn_action_9(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of the Special Forces command: those of its missions (core/specops).
uint16_t svc_specfrc(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
