// Delivering a supplies request to an objective, and the services every delivery report uses.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

// The services the delivery routines (opeu_request_action0, opeu_request_action1) call through their event.
enum class DeliveryService : uint16_t {
  LOCATION     = 0, // opeu_arrival_location(x, y, side): the map at the objective
  MESSAGE_TEXT = 1, // opeu_message_lookup(id) -> near pointer of the text
  FORMAT       = 2, // request_format(destination, format, words...)
  MESSAGE      = 3, // opeu_control_message(text, person)
  WAIT         = 4, // opeu_request_wait
};

void opeu_request_action0(uint8_t* ds, HostEvent event, uint8_t side, uint16_t request);

// The services of the delivery routines; module names the caller in diagnostics.
uint16_t svc_delivery(const char* module, uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
uint16_t svc_reqres(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
