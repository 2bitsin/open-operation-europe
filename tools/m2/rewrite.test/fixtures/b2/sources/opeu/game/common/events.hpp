// The fixture's callback types.
#pragma once

#include <cstdint>

class Game;

using HostEvent   = std::uint16_t (*)(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
using BattleEvent = std::uint16_t (*)(Game& game, std::uint32_t service, std::uint16_t const* args, unsigned count);
