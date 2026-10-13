// Opens the game's data files and resets the DOS disk buffers.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_open_resource(Game& game, HostEvent event, std::uint16_t name) -> std::int16_t;
void opeu_disk_reset(Game& game, HostEvent event);

// The module's services: the direct calls behind its events.
auto svc_resopen(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t;
