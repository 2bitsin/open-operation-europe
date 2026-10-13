// The final turn action of the selected-turn dispatch. It runs the current order's handler from the order handler
// table (indexed by the order code); an unfinished order counts its hold level down, then its movement level up by
// (99 - average value9) / 35 + 1, and moves with the code's repeat flag. A missing destination clears both order
// slots of the regiment.
#include "units/finalact.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "map/moveunit.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "map/movetgt.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"
#include "units/order-handler.hpp"
#include "units/orders.hpp"

using namespace opeu::game;

namespace {

// The progress an order is given when its hold level runs out.
constexpr std::uint8_t PROGRESS_HELD_OUT{ 200 };
} // namespace

// The order slot and its code are taken before the destination.
void opeu_turn_final_action(Game& game) {
  auto&             data{ Data(game) };
  ServiceCell const destination_cell;
  auto const        x{ destination_cell.XAt() };
  auto const        y{ destination_cell.YAt() };
  auto const        slot{ Near(data, Record<Unit>(data, data.current_unit_ptr).orders[0]) };
  auto const        code{ Record<Order>(data, slot).kind };
  if (!opeu_unit_order_destination(game, data.current_unit_ptr, game.State().Bytes(x, 1).data(), game.State().Bytes(y, 1).data())) {
    opeu_clear_unit_orders(game, data.current_unit_ptr, static_cast<std::uint8_t>(3));
    return;
  }
  if (static_cast<std::uint16_t>(
          port_call(game, static_cast<std::uint32_t>(static_cast<std::uint16_t>(data.order_handlers[code].segment)) * 16u + static_cast<std::uint16_t>(data.order_handlers[code].offset), 0, 0)))
    return;
  if (opeu_move_level_lower(game, slot, static_cast<uint8_t>(0))) Record<Order>(data, slot).progress = PROGRESS_HELD_OUT;
  std::uint16_t const unit[1]{ data.current_unit_ptr };
  std::uint16_t const raise[2]{ slot, static_cast<std::uint8_t>(static_cast<std::uint16_t>(99 - opeu_unit_average9(game, unit[0])) / 35 + 1) };
  if (opeu_move_level_raise(game, raise[0], static_cast<std::uint8_t>(raise[1]))) {
    opeu_move_unit(game, data.order_moves[code]);
  }
}
