// The message that tells the player why a filed request could not be carried out.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_request_error_present(Game& game, HostEvent event);

// The services opeu_request_error_present calls.
uint16_t svc_reqerror(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
