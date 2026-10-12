// The final turn action: runs the current order of the selected regiment.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_turn_final_action(uint8_t* ds, HostEvent event, uint16_t sp);

// The services the module calls out to.
uint16_t svc_finalact(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
