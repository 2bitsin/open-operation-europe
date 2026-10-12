// The general assignment screens before play.
#pragma once

#include "types.hpp"
#include "common/events.hpp"

void opeu_general_screen(uint8_t* ds, HostEvent event);
void opeu_reinforce_screen(uint8_t* ds, HostEvent event);
void opeu_play_screen(uint8_t* ds, HostEvent event);
void opeu_confirm_generals(uint8_t* ds, HostEvent event);

// The services the screens call through their event.
uint16_t svc_genconf(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count);
