// Regiment order helpers of the Orders command: cancelling a regiment's orders, the predicate of the regiment rows,
// and placing the items of the order menus and of the two-choice menus at their fixed spots.
#include "units/ordunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/order.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/orders.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {
// The order services the helpers call (served by svc_orders).
constexpr std::uint16_t CONFIRM_MESSAGE{ 2 }; // (text, person) -> nonzero when confirmed
// The two-choice menus.
constexpr std::uint16_t SUPPLY_MENU{ 2 };
constexpr std::uint16_t REPAIR_MENU{ 3 };
// The slots mask of both order slots.
constexpr std::uint8_t BOTH_SLOTS{ 3 };
} // namespace

// Asks to cancel a regiment's pending order and clears its order slots when confirmed.
void opeu_cancel_unit_orders(uint8_t ds[65536], HostEvent event, uint16_t unit, uint16_t person) {
  auto& data{ Data(ds) };
  if (Record<Unit>(data, unit).orders[0].kind != ORDER_UNUSED) {
    std::uint16_t const args[]{ Near(data, data.cancel_orders_prompt), person };
    if (event(ds, CONFIRM_MESSAGE, args, 2)) opeu_clear_unit_orders(ds, unit, BOTH_SLOTS);
  }
}

// Regiment row predicate: a ready regiment that may be selected.
uint8_t opeu_supply_unit_enabled(uint8_t ds[65536]) {
  auto&      data{ Data(ds) };
  auto const unit{ data.row_units[data.list_row] };
  return (Record<Unit>(data, unit).flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(ds, unit);
}

// Places the eight items of order menu `kind` at the order spots.
void opeu_order_layout(uint8_t ds[65536], uint8_t kind) {
  auto& data{ Data(ds) };
  for (std::uint16_t index{ 0 }; index < 8; ++index) {
    auto& item{ data.order_menus[kind][index] };
    item.x = data.order_spots[index].x;
    item.y = data.order_spots[index].y;
  }
}

// Places the two items of the two-choice menu `kind` (supply or repair) at the choice spots.
void opeu_order_layout_pair(uint8_t ds[65536], uint16_t kind) {
  auto& data{ Data(ds) };
  for (std::uint16_t index{ 0 }; index < 2; ++index) {
    auto const& spot{ data.choice_spots[index] };
    if (kind == SUPPLY_MENU) {
      data.supply_choices[index].x = spot.x;
      data.supply_choices[index].y = spot.y;
    }
    if (kind == REPAIR_MENU) {
      data.repair_choices[index].x = spot.x;
      data.repair_choices[index].y = spot.y;
    }
  }
}

// The helpers' only call, confirm_message, is an order service; the callers (ordissue, ordact) pass their own event.
uint16_t svc_ordunit(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case CONFIRM_MESSAGE: svc_orders(ds, id, args, count, &r); break;
  default:              port_unhandled("ordunit", id, count);
  }
  return r;
}

auto port_routine_opeu_supply_unit_enabled(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_supply_unit_enabled(ds);
}
