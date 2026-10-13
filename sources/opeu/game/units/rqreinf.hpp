// Regiment reinforcements of the request menu.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_reinf_element(Game& game, HostEvent event, uint16_t element, uint8_t kind, uint8_t side);
void opeu_request_regiment(Game& game, HostEvent event);

uint16_t svc_rqreinf(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
