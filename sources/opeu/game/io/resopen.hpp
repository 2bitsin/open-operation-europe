// Opens the game's data files and resets the DOS disk buffers.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_open_resource(std::uint8_t* ds, HostEvent event, std::uint16_t name)
    -> std::int16_t;
void opeu_disk_reset(std::uint8_t* ds, HostEvent event);

// The module's services: the direct calls behind its events.
auto svc_resopen(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args,
                 std::uint16_t count) -> std::uint16_t;
