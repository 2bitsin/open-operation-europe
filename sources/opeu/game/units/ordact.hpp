// The map cursor's unit action and the services of the order dialogs.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_unit_action(Game& game, HostEvent event, uint16_t unit, uint16_t unused);

uint16_t svc_ordact(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
int      svc_orders(Game& game, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result);
