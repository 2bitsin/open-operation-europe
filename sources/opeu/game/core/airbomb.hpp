// The Bombing Attack command, and svc_air, the services of the air force's commands.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_air_bomb_check(Game& game, uint8_t city, uint16_t type);
uint16_t opeu_air_bomb_order(Game& game, uint8_t x, uint8_t y, uint16_t type);
void     opeu_air_bomb_command(Game& game);
