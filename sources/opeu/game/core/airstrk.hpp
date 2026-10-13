// The air strike: its hourly countdown, the flight and the bombs.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

uint16_t opeu_air_damage_roll(Game& game, HostEvent event, uint8_t x, uint8_t y, uint8_t n);
void     opeu_air_destroyed_message(Game& game, HostEvent event, uint16_t what);
uint16_t opeu_air_strike_due(Game& game, HostEvent event);
void     opeu_air_strike_present(Game& game, HostEvent event);
void     opeu_air_strike_hit(Game& game, HostEvent event, uint8_t x, uint8_t y, uint16_t n);
void     opeu_air_strike_event(Game& game, HostEvent event);

// Runs the services the air strike calls through its event.
uint16_t svc_airstrk(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
