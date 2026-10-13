// A regiment's order slots: clear, set, restore and the destination of the current order.
#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>
#include <optional>

namespace opeu::game::detail::orders {

// The slots argument of opeu_clear_unit_orders: a mask of the slots to clear.
enum OrderSlots : std::uint8_t {
  CURRENT_SLOT = 1,
  SAVED_SLOT   = 2,
};

// A new current order: against the record `target`, or (target 0) at `cell`; save_current keeps the order it replaces
// in the saved slot.
struct NewOrder {
  std::uint16_t target;
  GridCell      cell;
  bool          save_current;
  std::uint8_t  code;
};

// Where a regiment's current order leads.
struct OrderDestination {
  std::optional<GridCell> cell;  // none for an order without a place
  bool                    valid; // the cell is on the map and the order's objective or target regiment is still there
};

// The destination of the regiment's current order: the order's cell, or its target regiment's cell; an objective order
// needs an objective at the cell, a regiment order a ready target regiment.
[[nodiscard]] auto UnitOrderDestination(Game const& game, std::uint16_t unit) -> OrderDestination;
} // namespace opeu::game::detail::orders

namespace opeu::game {
using detail::orders::CURRENT_SLOT;
using detail::orders::NewOrder;
using detail::orders::OrderDestination;
using detail::orders::OrderSlots;
using detail::orders::SAVED_SLOT;
using detail::orders::UnitOrderDestination;
} // namespace opeu::game

void opeu_clear_unit_orders(Game& game, uint16_t unit, uint8_t slots);
void opeu_set_unit_order(Game& game, uint16_t unit, opeu::game::NewOrder const& next);
void opeu_restore_unit_order(Game& game, uint16_t unit);
