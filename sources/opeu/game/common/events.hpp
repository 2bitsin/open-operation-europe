// The service callbacks through which game routines leave their module: each takes the game, a service id and the
// argument words, and returns the service's result (AX, or DX:AX for the battle and UI services).
#pragma once

#include <cstdint>

namespace opeu::game::detail::game {
class Game; // common/game.hpp
} // namespace opeu::game::detail::game

namespace opeu::game {
using detail::game::Game;
} // namespace opeu::game

// The game's routines are global functions until #17 moves them into their namespaces; they name the game unqualified.
using opeu::game::Game;

using HostEvent      = std::uint16_t (*)(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
using HostWordsEvent = std::uint16_t (*)(Game& game, std::uint16_t kind, std::uint16_t* args,
                                         std::uint16_t count); // the service may write results into args
using HostService    = std::uint16_t (*)(Game& game, std::uint16_t call, std::uint16_t* words);
using HostNotify     = void (*)(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
using BattleEvent    = std::uint16_t (*)(Game& game, std::uint32_t service, std::uint16_t const* args, unsigned count);
using BattleService  = std::uint16_t (*)(Game& game, std::uint16_t id, std::uint16_t arg);
using PendingEvent   = std::uint16_t (*)(Game& game, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t);
using SideColorEvent = void (*)(Game& game, std::uint16_t side);
using UiEvent        = std::uint32_t (*)(Game& game, std::uint8_t* memory, std::uint16_t id, std::uint16_t const* args, std::uint16_t count);
