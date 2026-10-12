#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_battle_draw_border(uint8_t* ds, HostEvent event);
uint8_t opeu_hex_terrain_code(uint8_t* ds, HostEvent event, uint8_t code);
void    opeu_battle_load_terrain(uint8_t* ds, HostEvent event);
void    opeu_battle_draw_map(uint8_t* ds, HostEvent event);
void    opeu_battle_draw_title(uint8_t* ds, HostEvent event);
void    opeu_battle_garrison_cells(uint8_t* ds, HostEvent event);
void    opeu_battle_reset_state(uint8_t* ds, HostEvent event);
void    opeu_battle_map_setup(uint8_t* ds, HostEvent event);

// The services of this module.
uint16_t svc_btlmap(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count);
