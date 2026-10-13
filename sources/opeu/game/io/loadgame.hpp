// The load menu and the saved-game checksum.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_saved_game_valid(Game& game, std::uint8_t slot) -> std::uint16_t;
auto opeu_load_saved_game(Game& game) -> std::uint16_t;
