// What a regiment's order slot holds: the order kinds (they index order_handlers and order_moves, and order_names
// from its fourth entry) and the value of an unused kind or progress byte.
#pragma once

#include <cstdint>

namespace opeu::game::detail::order {

// Order::kind of a regiment's order. The handler of each kind is order_handlers[kind].
enum class OrderKind : std::uint8_t {
  MOVE           = 0,  // to a cell
  OCCUPY         = 1,  // the enemy objective at a cell
  ATTACK         = 2,  // an enemy regiment
  DEFEND         = 3,  // a cell
  LAY_MINES      = 4,  // at an adjacent cell
  CLEAR_MINES    = 5,  // at an adjacent cell
  BUILD_BRIDGE   = 6,  // over the river at an adjacent cell
  DESTROY_BRIDGE = 7,  // at an adjacent cell
  SUPPLY_CITY    = 8,  // the own objective at a cell
  SUPPLY_UNIT    = 9,  // a friendly regiment
  REPAIR_CITY    = 10, // the arms of the own objective at a cell
  REPAIR_UNIT    = 11, // the arms of a friendly regiment
  PURSUE         = 12, // an enemy regiment
  REST           = 13, // fall back to a cell
  NONE           = 0xff,
};

// The number of order kinds (the size of order_handlers and order_moves).
inline constexpr std::uint8_t ORDER_KINDS{ 14 };

// Order::kind and Order::progress of an unused order slot.
inline constexpr std::uint8_t ORDER_UNUSED{ 0xff };

// The order kind as the byte the slot stores.
[[nodiscard]] constexpr auto Kind(OrderKind kind) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(kind);
}

// Whether an order of `kind` targets a regiment (its slot holds the unit's near pointer) rather than a cell.
[[nodiscard]] constexpr auto TargetsUnit(std::uint8_t kind) noexcept -> bool {
  switch (static_cast<OrderKind>(kind)) {
    case OrderKind::ATTACK:
    case OrderKind::SUPPLY_UNIT:
    case OrderKind::REPAIR_UNIT:
    case OrderKind::PURSUE:      return true;
    default:                     return false;
  }
}
} // namespace opeu::game::detail::order

namespace opeu::game {
using detail::order::Kind;
using detail::order::ORDER_KINDS;
using detail::order::ORDER_UNUSED;
using detail::order::OrderKind;
using detail::order::TargetsUnit;
} // namespace opeu::game
