// Regiment order helpers of the Orders command: cancelling a regiment's orders, the predicate of the regiment rows,
// and placing the items of the order menus and of the two-choice menus at their fixed spots.
#include "units/ordunit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/order.hpp"
#include "core/dialogs.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/orders.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/ordact.hpp"

using namespace opeu::game;

namespace {
// The two-choice menus.
constexpr std::uint16_t SUPPLY_MENU{ 2 };
constexpr std::uint16_t REPAIR_MENU{ 3 };
// The slots mask of both order slots.
constexpr std::uint8_t BOTH_SLOTS{ 3 };
} // namespace

// Asks to cancel a regiment's pending order and clears its order slots when confirmed.
void opeu_cancel_unit_orders(Game& game, uint16_t unit, uint16_t person) {
  auto& data{ Data(game) };
  if (Record<Unit>(data, unit).orders[0].kind != ORDER_UNUSED) {
    if (opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.cancel_orders_prompt), person)) opeu_clear_unit_orders(game, unit, BOTH_SLOTS);
  }
}

// Regiment row predicate: a ready regiment that may be selected.
uint8_t opeu_supply_unit_enabled(Game& game) {
  auto&      data{ Data(game) };
  auto const unit{ data.row_units[data.list_row] };
  return (Record<Unit>(data, unit).flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(game, unit);
}

// Places the eight items of order menu `kind` at the order spots.
void opeu_order_layout(Game& game, uint8_t kind) {
  auto& data{ Data(game) };
  for (std::uint16_t index{ 0 }; index < 8; ++index) {
    auto& item{ data.order_menus[kind][index] };
    item.x = data.order_spots[index].x;
    item.y = data.order_spots[index].y;
  }
}

// Places the two items of the two-choice menu `kind` (supply or repair) at the choice spots.
void opeu_order_layout_pair(Game& game, uint16_t kind) {
  auto& data{ Data(game) };
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
