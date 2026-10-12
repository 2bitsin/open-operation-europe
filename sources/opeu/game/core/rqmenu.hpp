// The request menu (turn action 7): the table of the side's requests and the choice of supplies, a battalion or a
// regiment.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_request_header(uint8_t* ds, HostEvent event);
void opeu_turn_action_7(uint8_t* ds, HostEvent event);

// The services of opeu_turn_action_7 (the request menu's, core/rqcommon).
uint16_t svc_rqmenu(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
