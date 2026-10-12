// The Reorganize command.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_turn_action_6(uint8_t* ds, HostEvent event, uint16_t frame,
                        uint16_t segment);

uint16_t svc_organize(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
