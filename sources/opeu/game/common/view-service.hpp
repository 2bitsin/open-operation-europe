// The helpers the information views (core/vwarms, vwcity, vwdiv, vwpanel, vwpeople, vwregt and units/vwlist) share.
// Pointer arguments are DS offsets.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"

#include <cstdint>

namespace opeu::game::detail::view_service {

// The DS stack size of the list window state a view saves around a nested list dialog.
inline constexpr std::uint16_t LIST_WINDOW_STATE{ 7 };
} // namespace opeu::game::detail::view_service

namespace opeu::game {
using detail::view_service::LIST_WINDOW_STATE;
} // namespace opeu::game
