// The new-game setup: scenario, players, sides, map, battles and difficulty.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

int opeu_new_game_setup(uint8_t* ds, HostEvent event, uint16_t sp);

// The services of this module in the native build.
uint16_t svc_scensel(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
