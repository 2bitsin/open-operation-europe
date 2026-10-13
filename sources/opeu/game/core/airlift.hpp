// The Airlift command: the cargo amounts, drop point and delay of a supply airlift.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint8_t  opeu_air_lift_lead(Game& game, uint8_t hours, uint16_t planes);
uint16_t opeu_air_lift_order(Game& game, uint8_t x, uint8_t y, uint8_t hours, uint16_t planes, uint16_t ammo, uint16_t fuel, uint16_t food);
void     opeu_air_lift_command(Game& game);
