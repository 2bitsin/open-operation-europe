#pragma once

#include "types.hpp"
#include "types/events.hpp"

void     opeu_move_spot(uint8_t* ds, HostEvent event);
uint16_t opeu_move_enemy_adjacent(uint8_t* ds, HostEvent event, uint16_t bp);
void     opeu_move_mines_finish(uint8_t* ds, HostEvent event);

// The services of this module.
uint16_t svc_movespot(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
