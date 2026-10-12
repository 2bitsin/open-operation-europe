// The air-raid event of scenario 3.
#pragma once

#include "types.hpp"
#include "types/events.hpp"

void opeu_bomb_draw_frame(uint8_t* ds, HostEvent event, uint16_t load,
                          uint16_t x, uint16_t y);
void opeu_bomb_restore_frame(uint8_t* ds, HostEvent event, uint16_t load,
                             uint16_t x, uint16_t y);
void opeu_bomb_animate(uint8_t* ds, HostEvent event, uint16_t load, uint16_t bp,
                       uint16_t unit, uint8_t side);
void opeu_bomb_present(uint8_t* ds, HostEvent event, uint16_t load);
void opeu_bomb_event(uint8_t* ds, HostEvent event, uint16_t load, uint16_t bp);

// Runs the services the air raid calls through its event.
uint16_t svc_evbomb(uint8_t* ds, uint16_t id, const uint16_t* args,
                    uint16_t count);
