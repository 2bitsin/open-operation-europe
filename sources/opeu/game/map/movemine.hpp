#pragma once

#include "types.hpp"
#include "types/events.hpp"

void    opeu_move_mines(uint8_t* ds, HostEvent event, uint8_t x, uint8_t y);
void    opeu_move_clamp(uint8_t* ds);
uint8_t opeu_move_rating(uint8_t* ds, HostEvent event);

// The services of this module.
uint16_t svc_movemine(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count);
