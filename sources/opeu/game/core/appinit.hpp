// The startup stub and the first-run input-device prompt.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_app_startup(uint8_t* ds, HostEvent event);
void opeu_empty_routine_a(void);
void opeu_empty_routine_b(void);
void opeu_app_initialize(uint8_t* ds, HostEvent event);

// The services of this module as direct calls.
uint16_t svc_appinit(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
