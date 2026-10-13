// The startup stub and the first-run input-device prompt.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_app_startup(Game& game, HostEvent event);
void opeu_empty_routine_a(void);
void opeu_empty_routine_b(void);
void opeu_app_initialize(Game& game, HostEvent event);

// The services of this module as direct calls.
uint16_t svc_appinit(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
