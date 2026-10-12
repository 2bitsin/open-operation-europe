// The message that tells the player why a filed request could not be carried out.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_request_error_present(uint8_t* ds, HostEvent event);

// The services opeu_request_error_present calls.
uint16_t svc_reqerror(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
