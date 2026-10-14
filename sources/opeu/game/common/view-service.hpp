// The helpers the information views (core/vwarms, vwcity, vwdiv, vwpanel, vwpeople, vwregt and units/vwlist) share.
// Pointer arguments are DS offsets.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace opeu::game::detail::view_service {

// The list window state a view keeps aside around a nested list dialog: the active window, its column, top row, columns,
// rows and the byte after them.
inline constexpr std::uint16_t LIST_WINDOW_STATE{ 7 };
using ListWindowState = std::array<std::uint8_t, LIST_WINDOW_STATE>;

[[nodiscard]] inline auto SaveListWindow(Game const& game) -> ListWindowState {
  ListWindowState state{};
  std::ranges::copy(game.State().Bytes(offsetof(DataSegment, list_window), LIST_WINDOW_STATE), state.begin());
  return state;
}

inline auto RestoreListWindow(Game& game, ListWindowState const& state) -> void {
  std::ranges::copy(state, game.State().Bytes(offsetof(DataSegment, list_window), LIST_WINDOW_STATE).begin());
}
} // namespace opeu::game::detail::view_service

namespace opeu::game {
using detail::view_service::LIST_WINDOW_STATE;
using detail::view_service::ListWindowState;
using detail::view_service::RestoreListWindow;
using detail::view_service::SaveListWindow;
} // namespace opeu::game
