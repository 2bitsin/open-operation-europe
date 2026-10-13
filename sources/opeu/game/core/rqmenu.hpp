// The request menu (turn action 7): the table of the side's requests and the choice of supplies, a battalion or a
// regiment.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_request_header(Game& game);
void opeu_turn_action_7(Game& game, HostEvent event);

// The services of opeu_turn_action_7 (the request menu's, core/rqcommon).
uint16_t svc_rqmenu(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
