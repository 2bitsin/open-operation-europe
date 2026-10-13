// The Game the entry runs, for the one path that reaches a game routine without being handed one: the picture decoder
// (services/vgalib) pulls UIPRIM's stream bytes through a plain function pointer (link/hosts.cpp). Interim, for M2's
// batch B1 only: B4 gives the decoder a reader that carries its Game, and this file is deleted.
#pragma once

#include "common/events.hpp"

namespace opeu::game::detail::entry_game {

// Makes `game` the running game for the scope of this object (the entry holds one for the whole run).
class EntryGame {
public:
  explicit EntryGame(Game& game) noexcept;
  EntryGame(EntryGame const&)                    = delete;
  EntryGame(EntryGame&&)                         = delete;
  auto operator=(EntryGame const&) -> EntryGame& = delete;
  auto operator=(EntryGame&&) -> EntryGame&      = delete;
  ~EntryGame();

  // The running game; a contract failure when the entry has not started one.
  [[nodiscard]] static auto Current() -> Game&;
};
} // namespace opeu::game::detail::entry_game

namespace opeu::game {
using detail::entry_game::EntryGame;
} // namespace opeu::game
