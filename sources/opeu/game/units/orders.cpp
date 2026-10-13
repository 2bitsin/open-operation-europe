// A regiment's two order slots: the current order and the order saved while it carries out another. A slot holds the
// target (a near pointer to an objective or regiment, or a map cell), the order kind and its progress; an unused kind
// or progress byte is 0xff.
#include "units/orders.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/order.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include <array>
#include <ranges>

using namespace opeu::game;

// Clears the slots in the mask and the regiment's target and movement state.
void opeu_clear_unit_orders(Game& game, uint16_t unit, uint8_t slots) {
  auto& regiment{ Record<Unit>(Data(game), unit) };
  for (auto const [order, mask] : std::views::zip(regiment.orders, std::array{ CURRENT_SLOT, SAVED_SLOT }))
    if (slots & mask) order.progress = order.kind = ORDER_UNUSED;
  regiment.target_x = ORDER_UNUSED;
  regiment.moved    = 0;
}

// Gives the regiment an order of kind `code` at `target` (or, when target is 0, at the cell x, y). With save_current
// the current order moves to the saved slot first; otherwise the saved slot is cleared.
void opeu_set_unit_order(Game& game, uint16_t unit, NewOrder const& next) {
  auto const [target, cell, save_current, code]{ next };
  auto& regiment{ Record<Unit>(Data(game), unit) };
  auto& order{ regiment.orders[0] };
  if (save_current) {
    regiment.orders[1] = order;
    opeu_clear_unit_orders(game, unit, CURRENT_SLOT);
  } else
    opeu_clear_unit_orders(game, unit, SAVED_SLOT);
  if (target)
    order.target = target;
  else {
    order.cell.x = cell.x;
    order.cell.y = cell.y;
  }
  order.kind     = code;
  order.progress = ORDER_UNUSED;
}

// Brings the saved order back into the current slot; without one, both slots are cleared.
void opeu_restore_unit_order(Game& game, uint16_t unit) {
  auto& regiment{ Record<Unit>(Data(game), unit) };
  if (regiment.orders[1].kind != ORDER_UNUSED) {
    regiment.orders[0] = regiment.orders[1];
    opeu_clear_unit_orders(game, unit, SAVED_SLOT);
  } else
    opeu_clear_unit_orders(game, unit, CURRENT_SLOT | SAVED_SLOT);
}

namespace opeu::game::detail::orders {
namespace {
// Whether an order of `kind` at the cell needs an objective there.
[[nodiscard]] auto NeedsObjective(std::uint8_t kind) -> bool {
  return kind == Kind(UnitOrder::OCCUPY) || kind == Kind(UnitOrder::RESUPPLY) || kind == Kind(UnitOrder::REPAIR_CITY);
}

[[nodiscard]] auto OnMap(GridCell cell) -> bool {
  return cell.x < MAP_WIDTH && cell.y < MAP_HEIGHT;
}

[[nodiscard]] auto CellDestination(Game const& game, Order const& order) -> OrderDestination {
  GridCell const cell{ order.cell.x, order.cell.y };
  auto const     valid{ (!NeedsObjective(order.kind) || opeu_find_objective_at(game, cell.x, cell.y)) && OnMap(cell) };
  return { cell, valid };
}

[[nodiscard]] auto RegimentDestination(DataSegment const& data, Order const& order) -> OrderDestination {
  auto const&    target{ Record<Unit>(data, order.target) };
  GridCell const cell{ target.x, target.y };
  auto const     valid{ order.target && (target.flags & Bits(UnitFlag::READY)) && OnMap(cell) };
  return { cell, valid };
}
} // namespace

auto UnitOrderDestination(Game const& game, std::uint16_t unit) -> OrderDestination {
  auto const& data{ Data(game) };
  auto const& order{ Record<Unit>(data, unit).orders[0] };
  switch (static_cast<UnitOrder>(order.kind)) {
  case UnitOrder::MOVE:
  case UnitOrder::OCCUPY:
  case UnitOrder::DEFEND:
  case UnitOrder::LAY_MINES:
  case UnitOrder::CLEAR_MINES:
  case UnitOrder::BUILD_BRIDGE:
  case UnitOrder::DESTROY_BRIDGE:
  case UnitOrder::RESUPPLY:
  case UnitOrder::REPAIR_CITY:
  case UnitOrder::REST:           return CellDestination(game, order);
  case UnitOrder::ATTACK:
  case UnitOrder::SUPPLY_UNIT:
  case UnitOrder::REPAIR_UNIT:
  case UnitOrder::PURSUE:         return RegimentDestination(data, order);
  default:                        return { std::nullopt, false };
  }
}
} // namespace opeu::game::detail::orders
