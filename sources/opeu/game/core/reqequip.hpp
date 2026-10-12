// Delivering a battalion request: equipment of the requested type added to an item slot of the objective.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_request_action1(uint8_t* ds, HostEvent event, uint8_t side,
                          uint16_t request);

// The services of opeu_request_action1 (the delivery services of core/reqres).
uint16_t svc_reqequip(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
