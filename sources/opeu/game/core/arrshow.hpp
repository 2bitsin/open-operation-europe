// The arrival message.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_arrival_present(Game& game, HostEvent event);

// The services of this module as direct calls.
uint16_t svc_arrshow(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
