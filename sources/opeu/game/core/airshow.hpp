// The air-operation display: the plane sprite and the weather report before an operation.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void    opeu_air_sprite_mirror(Game& game, uint16_t image, uint8_t keep);
void    opeu_air_sprite_draw(Game& game, HostEvent event, uint16_t load, uint16_t x, uint16_t y, uint8_t keep);
void    opeu_air_sprite_restore(Game& game, HostEvent event, uint16_t load, uint16_t x, uint16_t y);
uint8_t opeu_air_report(Game& game, HostEvent event, uint16_t what, uint8_t wait);

// The services of this module as direct calls.
uint16_t svc_airshow(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
