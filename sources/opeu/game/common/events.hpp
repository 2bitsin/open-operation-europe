// The game's forward declaration, for the headers that name `Game` without needing its definition. (The service
// callback types that lived here went with the last svc_* dispatcher in B3.)
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
