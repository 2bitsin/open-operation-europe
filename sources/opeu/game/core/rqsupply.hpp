// The request menu's supplies choice, and the row predicate of the battalion type list.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_request_supplies(uint8_t* ds, HostEvent event, uint16_t scratch);
uint8_t opeu_rq_type_selectable();

// The services of opeu_request_supplies (the request menu's, core/rqcommon).
uint16_t svc_rqsupply(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
