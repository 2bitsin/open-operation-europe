// The air force's dialog helpers shared by the bombing, paratroop and airlift commands.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

uint16_t opeu_air_ask_count(uint8_t* ds, HostEvent event, uint16_t sp,
                            uint16_t text, uint16_t low, uint16_t high,
                            int cancel, uint16_t person, uint16_t items);
uint16_t opeu_air_free_planes(uint8_t* ds, HostEvent event, uint16_t sp);
uint16_t opeu_air_target_valid(uint8_t* ds, HostEvent event, uint16_t sp,
                               uint8_t x, uint8_t y, uint16_t what);
uint16_t opeu_air_confirm(uint8_t* ds, HostEvent event, uint16_t sp);

// Runs the services the module's routines call through their event.
uint16_t svc_airhelp(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
