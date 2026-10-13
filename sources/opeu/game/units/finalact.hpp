// The final turn action: runs the current order of the selected regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_turn_final_action(Game& game, HostEvent event);

// The services the module calls out to.
uint16_t svc_finalact(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
