// The list selection dialogs.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"

#include <cstdint>

namespace opeu::game::detail::listdlg {

// How a list dialog runs: how many choices it takes (limit) and its area and activation flags.
struct ListOptions {
  std::uint8_t limit;
  std::uint8_t flags;
};

// What most list dialogs of the game are run with: one choice, flags 7.
inline constexpr ListOptions LIST_OPTIONS{ .limit = 1, .flags = 7 };
} // namespace opeu::game::detail::listdlg

namespace opeu::game {
using detail::listdlg::LIST_OPTIONS;
using detail::listdlg::ListOptions;
} // namespace opeu::game

// The commander dialog: the generals picked for the play screen (ignores the limit).
auto opeu_commander_list_dialog(Game& game, opeu::game::ListWindow const& window, opeu::game::ListOptions options) -> std::uint16_t;
// The list dialog of the window: the row chosen (1-based), 0 when cancelled.
auto opeu_list_dialog(Game& game, opeu::game::ListWindow const& window, opeu::game::ListOptions options) -> std::uint16_t;
