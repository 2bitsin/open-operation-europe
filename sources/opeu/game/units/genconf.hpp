// The general assignment screens before play.
#pragma once

#include "common/events.hpp"
#include "types.hpp"

void opeu_general_screen(Game& game, HostEvent event);
void opeu_reinforce_screen(Game& game, HostEvent event);
void opeu_play_screen(Game& game, HostEvent event);
void opeu_confirm_generals(Game& game, HostEvent event);

// The services the screens call through their event.
uint16_t svc_genconf(Game& game, uint16_t id, const uint16_t* args, uint16_t count);
