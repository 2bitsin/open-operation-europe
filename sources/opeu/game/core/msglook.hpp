// Looking up a battle message into the shared message buffer.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_message_lookup(uint8_t* ds, HostEvent event, int16_t id);

// The services opeu_message_lookup is called with.
uint16_t svc_msglook(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
