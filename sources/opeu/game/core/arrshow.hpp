// The arrival message.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_arrival_present(uint8_t* ds, HostEvent event, uint16_t scratch);

// The services of this module as direct calls.
uint16_t svc_arrshow(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
