// A regiment's two order slots: the current order and the order saved while it carries out another. A slot holds the
// target (a near pointer to an objective or regiment, or a map cell), the order kind and its progress; an unused kind
// or progress byte is 0xff.
#include "types.hpp"
#include "units/orders.hpp"
#include "common/data-segment.hpp"
#include "common/order.hpp"
#include "map/query.hpp"

using namespace opeu::game;

namespace {
// The slots argument of opeu_clear_unit_orders: a mask of the slots to clear.
enum Slots : std::uint8_t {
  CURRENT_SLOT = 1,
  SAVED_SLOT   = 2,
};
} // namespace

// Clears the slots in the mask and the regiment's target and movement state.
void opeu_clear_unit_orders(uint8_t ds[65536], uint16_t unit, uint8_t slots) {
  auto& regiment{ Record<Unit>(Data(ds), unit) };
  for (std::uint8_t slot{ 0 }; slot < 2; ++slot)
    if (slots & (1u << slot))
      regiment.orders[slot].progress = regiment.orders[slot].kind =
          ORDER_UNUSED;
  regiment.target_x = ORDER_UNUSED;
  regiment.moved    = 0;
}

// Gives the regiment an order of kind `code` at `target` (or, when target is 0, at the cell x, y). With save_current
// the current order moves to the saved slot first; otherwise the saved slot is cleared.
void opeu_set_unit_order(uint8_t ds[65536], uint16_t unit, uint16_t target,
                         uint8_t x, uint16_t save_current, uint8_t code,
                         uint8_t y) {
  auto& regiment{ Record<Unit>(Data(ds), unit) };
  auto& order{ regiment.orders[0] };
  if (save_current) {
    regiment.orders[1] = order;
    opeu_clear_unit_orders(ds, unit, CURRENT_SLOT);
  } else
    opeu_clear_unit_orders(ds, unit, SAVED_SLOT);
  if (target)
    order.target = target;
  else {
    order.cell.x = x;
    order.cell.y = y;
  }
  order.kind     = code;
  order.progress = ORDER_UNUSED;
}

// Brings the saved order back into the current slot; without one, both slots are cleared.
void opeu_restore_unit_order(uint8_t ds[65536], uint16_t unit) {
  auto& regiment{ Record<Unit>(Data(ds), unit) };
  if (regiment.orders[1].kind != ORDER_UNUSED) {
    regiment.orders[0] = regiment.orders[1];
    opeu_clear_unit_orders(ds, unit, SAVED_SLOT);
  } else
    opeu_clear_unit_orders(ds, unit, CURRENT_SLOT | SAVED_SLOT);
}

// The cell the regiment's current order leads to, in x and y; nonzero when it is on the map and still valid: an
// objective order needs an objective at the cell, a regiment order a ready target regiment. x and y are written even
// when the order turns out invalid.
uint16_t opeu_unit_order_destination(const uint8_t ds[65536], uint16_t unit,
                                     uint8_t* x, uint8_t* y) {
  auto const& data{ Data(ds) };
  auto const& order{ Record<Unit>(data, unit).orders[0] };
  auto const  kind{ order.kind };
  switch (static_cast<OrderKind>(kind)) {
    case OrderKind::MOVE:
    case OrderKind::OCCUPY:
    case OrderKind::DEFEND:
    case OrderKind::LAY_MINES:
    case OrderKind::CLEAR_MINES:
    case OrderKind::BUILD_BRIDGE:
    case OrderKind::DESTROY_BRIDGE:
    case OrderKind::SUPPLY_CITY:
    case OrderKind::REPAIR_CITY:
    case OrderKind::REST:
      *x = order.cell.x;
      *y = order.cell.y;
      if ((kind == Kind(OrderKind::OCCUPY) ||
           kind == Kind(OrderKind::SUPPLY_CITY) ||
           kind == Kind(OrderKind::REPAIR_CITY)) &&
          !opeu_find_objective_at(ds, *x, *y))
        return 0;
      break;
    case OrderKind::ATTACK:
    case OrderKind::SUPPLY_UNIT:
    case OrderKind::REPAIR_UNIT:
    case OrderKind::PURSUE:      {
      auto const& target{ Record<Unit>(data, order.target) };
      *x = target.x;
      *y = target.y;
      if (!order.target || !(target.flags & Bits(UnitFlag::READY)))
        return 0;
      break;
    }
    default: return 0;
  }
  return *x < MAP_WIDTH && *y < MAP_HEIGHT;
}
