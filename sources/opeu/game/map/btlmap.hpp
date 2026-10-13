#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_battle_draw_border(Game& game, HostEvent event);
uint8_t opeu_hex_terrain_code(Game& game, uint8_t code);
void    opeu_battle_load_terrain(Game& game, HostEvent event);
void    opeu_battle_draw_map(Game& game, HostEvent event);
void    opeu_battle_draw_title(Game& game, HostEvent event);
void    opeu_battle_garrison_cells(Game& game, HostEvent event);
void    opeu_battle_reset_state(Game& game);
void    opeu_battle_map_setup(Game& game, HostEvent event);

// The services of this module.
uint16_t svc_btlmap(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
