// The air strike: its hourly countdown, the flight and the bombs.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_air_damage_roll(uint8_t* ds, HostEvent event, uint8_t x,
                              uint8_t y, uint8_t n);
void     opeu_air_destroyed_message(uint8_t* ds, HostEvent event, uint16_t bp,
                                    uint16_t what);
uint16_t opeu_air_strike_due(uint8_t* ds, HostEvent event);
void     opeu_air_strike_present(uint8_t* ds, HostEvent event);
void opeu_air_strike_hit(uint8_t* ds, HostEvent event, uint16_t bp, uint8_t x,
                         uint8_t y, uint16_t n);
void opeu_air_strike_event(uint8_t* ds, HostEvent event, uint16_t bp);

// Runs the services the air strike calls through its event.
uint16_t svc_airstrk(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
