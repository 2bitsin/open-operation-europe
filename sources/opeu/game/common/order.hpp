// What a regiment's order slot holds beside its UnitOrder kind (common/data-segment.hpp): the number of kinds (they
// index order_handlers and order_moves, and order_names from its fourth entry), the value of an unused kind or
// progress byte, and which kinds target a regiment.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>

namespace opeu::game::detail::order {

// Order::kind and Order::progress of an unused order slot.
inline constexpr std::uint8_t ORDER_UNUSED{ 0xff };

// The slot mask of opeu_clear_unit_orders that clears both of a regiment's order slots.
inline constexpr std::uint8_t ALL_ORDER_SLOTS{ 0x03 };

// The order kind as the byte the slot stores.
[[nodiscard]] constexpr auto Kind(UnitOrder kind) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(kind);
}

// Whether the map shows an order of `kind` until the order has started (occupy, attack, defend, rest); the others
// show until they are complete.
[[nodiscard]] constexpr auto ShowsUntilStarted(std::uint8_t kind) noexcept -> bool {
  switch (static_cast<UnitOrder>(kind)) {
  case UnitOrder::OCCUPY:
  case UnitOrder::ATTACK:
  case UnitOrder::DEFEND:
  case UnitOrder::REST:   return true;
  default:                return false;
  }
}
} // namespace opeu::game::detail::order

namespace opeu::game {
using detail::order::Kind;
using detail::order::ORDER_UNUSED;
using detail::order::ShowsUntilStarted;
using detail::order::ALL_ORDER_SLOTS;
} // namespace opeu::game
