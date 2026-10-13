// The list selection dialogs.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_commander_list_dialog(Game& game, std::uint16_t window, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t;
auto opeu_list_dialog(Game& game, std::uint16_t window, std::uint8_t limit, std::uint8_t flags) -> std::uint16_t;
