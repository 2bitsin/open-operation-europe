// The fixture's menu: its sites are already direct, its callbacks only handed on.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void          opeu_menu_show(Game& game, HostEvent event, std::uint16_t row);
void          opeu_menu_run(Game& game, HostEvent event);
void          opeu_menu_hold(Game& game, HostEvent event);
std::uint16_t svc_menu(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
