// The fixture's callback types.
#pragma once

#include <cstdint>

class Game;

using HostEvent    = std::uint16_t (*)(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
using PendingEvent = std::uint16_t (*)(Game& game, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t);
