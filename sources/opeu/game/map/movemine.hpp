#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_move_mines(Game& game, HostEvent event, uint8_t x, uint8_t y);
void    opeu_move_clamp(Game& game);
uint8_t opeu_move_rating(Game& game, HostEvent event);

// The services of this module.
uint16_t svc_movemine(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
