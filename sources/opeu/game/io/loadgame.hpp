// The load menu and the saved-game checksum.
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_saved_game_valid(std::uint8_t* ds, std::uint8_t slot) -> std::uint16_t;
auto opeu_save_record_size() -> std::uint16_t;
auto opeu_load_saved_game(std::uint8_t* ds, HostEvent event) -> std::uint16_t;

// The module's services: the direct calls behind its events.
auto svc_loadgame(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t;
