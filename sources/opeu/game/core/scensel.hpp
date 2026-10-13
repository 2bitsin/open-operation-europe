// The new-game setup: scenario, players, sides, map, battles and difficulty.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

int opeu_new_game_setup(Game& game, HostEvent event);

// The services of this module in the native build.
uint16_t svc_scensel(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
