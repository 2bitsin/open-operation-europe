#include "link/entry-game.hpp"

#include "opeu/contract.hpp"

namespace opeu::game::detail::entry_game {
namespace {
// The running game, set by the entry's EntryGame. The one mutable global B1 adds, deleted with this file by B4.
Game* running{ nullptr }; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables): interim, see the header
} // namespace

EntryGame::EntryGame(Game& game) noexcept {
  running = &game;
}

EntryGame::~EntryGame() {
  running = nullptr;
}

auto EntryGame::Current() -> Game& {
  Expects(running != nullptr, "the entry runs a game");
  return *running;
}
} // namespace opeu::game::detail::entry_game
