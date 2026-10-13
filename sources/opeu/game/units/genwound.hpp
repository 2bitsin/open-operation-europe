// A general's chance of being wounded or killed.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_general_casualty(Game& game, HostEvent event, uint16_t person);

// The services the module calls out to.
uint16_t svc_genwound(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
