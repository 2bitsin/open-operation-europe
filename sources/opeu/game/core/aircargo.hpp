// The supply airlift's flight, and the hourly air operations.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

uint16_t opeu_cargo_drop_due(uint8_t* ds, HostEvent event);
void     opeu_cargo_distribute(uint8_t* ds, uint16_t cargo, uint16_t amount);
void     opeu_cargo_drop_present(uint8_t* ds, HostEvent event);
void     opeu_cargo_drop_event(uint8_t* ds, HostEvent event, uint16_t bp);
void     opeu_air_operations(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the air operations call through their event.
uint16_t svc_aircargo(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
