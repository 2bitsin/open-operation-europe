#pragma once

#include "common/events.hpp"
#include "types.hpp"

void     opeu_move_spot(Game& game, HostEvent event);
uint16_t opeu_move_enemy_adjacent(Game& game, HostEvent event);
void     opeu_move_mines_finish(Game& game, HostEvent event);

// The services of this module.
uint16_t svc_movespot(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
