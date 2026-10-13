// The Special Forces command: the officer who leads the missions, the pending mission and the mission menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_special_officer(Game& game, HostEvent event);
uint16_t opeu_special_unit_target(Game& game);
void     opeu_turn_action_9(Game& game, HostEvent event);

// The services of the Special Forces command: those of its missions (core/specops).
uint16_t svc_specfrc(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
