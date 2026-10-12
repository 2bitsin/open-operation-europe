// Regiment reinforcements of the request menu.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_reinf_element(uint8_t* ds, HostEvent event, uint16_t element,
                        uint8_t kind, uint8_t side);
void opeu_request_regiment(uint8_t* ds, HostEvent event);

uint16_t svc_rqreinf(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
