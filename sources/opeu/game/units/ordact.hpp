// The map cursor's unit action and the services of the order dialogs.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_unit_action(uint8_t* ds, HostEvent event, uint16_t sp, uint16_t unit,
                      uint16_t unused);

uint16_t svc_ordact(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
int svc_orders(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count,
               uint16_t* result);
