// The helpers the information views (core/vwarms, vwcity, vwdiv, vwpanel, vwpeople, vwregt and units/vwlist) share,
// and the row checks of their list dialogs. Pointer arguments are DS offsets.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "link/link.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>

namespace opeu::game::detail::view_service {

// The row checks of the views' list dialogs (units/vwlist), by their offset in the views' code segment: a list
// dialog offers only the rows its check accepts.
enum class RowCheck : std::uint16_t {
  OBJECTIVE = 0x0120, // own objectives that may be listed, enemy ones that are marked
  NONE      = 0x016e, // no row
  DIVISION  = 0x0172, // a present division whose selected regiment may be selected
  REGIMENT  = 0x01ac, // a present regiment that may be selected if ours, sighted if the enemy's
  NONE_2    = 0x0200, // no row
  NONE_3    = 0x0204, // no row
  ANY       = 0x0208, // every row
};

// The views' code segment, as the far call table numbers it: the segment of a row check's far pointer.
inline constexpr std::uint16_t VIEW_CODE_SEGMENT{ 0x1fe6 };

// The DS stack size of the list window state a view saves around a nested list dialog.
inline constexpr std::uint16_t LIST_WINDOW_STATE{ 7 };

// Gives a list dialog its row check.
inline auto SetRowCheck(Game& game, ListWindow& dialog, RowCheck check) -> void {
  dialog.select.offset  = std::to_underlying(check);
  dialog.select.segment = static_cast<std::uint16_t>(VIEW_CODE_SEGMENT);
}

// Runs the menu action `item` (1 first) of a far pointer table.
template <std::size_t N>
inline auto RunAction(Game& game, std::array<FarPointer, N> const& actions, std::uint16_t item) -> std::uint16_t {
  auto const& action{ actions[item - 1u] };
  return static_cast<uint16_t>(port_call(game, static_cast<uint32_t>(static_cast<std::uint16_t>(action.segment)) * 16u + static_cast<std::uint16_t>(action.offset), nullptr, 0));
}

} // namespace opeu::game::detail::view_service

namespace opeu::game {
using detail::view_service::LIST_WINDOW_STATE;
using detail::view_service::RowCheck;
using detail::view_service::RunAction;
using detail::view_service::SetRowCheck;
using detail::view_service::VIEW_CODE_SEGMENT;
} // namespace opeu::game
