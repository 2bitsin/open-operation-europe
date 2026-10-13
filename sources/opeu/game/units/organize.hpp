// The Reorganize command.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_turn_action_6(Game& game, HostEvent event, uint16_t segment);

uint16_t svc_organize(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
