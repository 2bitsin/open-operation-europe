// Opens the game's data files and resets the DOS disk buffers.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_open_resource(Game& game, std::uint16_t name) -> std::int16_t;
void opeu_disk_reset(Game& game);
