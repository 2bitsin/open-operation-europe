// The final turn action of the selected-turn dispatch. It runs the current order's handler from the order handler
// table (indexed by the order code); an unfinished order counts its hold level down, then its movement level up by
// (99 - average value9) / 35 + 1, and moves with the code's repeat flag. A missing destination clears both order
// slots of the regiment.
#include "types.hpp"
#include "units/finalact.hpp"
#include "common/data-segment.hpp"
#include "common/order-service.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "map/movetgt.hpp"
#include "units/engineer.hpp"
#include "units/orders.hpp"

using namespace opeu::game;

namespace {
// The calls the action makes through its event (served by svc_finalact).
enum Service : std::uint16_t {
  DESTINATION =
      0, // (unit, &x, &y): the target cell of the unit's order, 0 when there is none
  LEVEL_LOWER  = 6,  // (slot, 0)
  CLEAR_ORDERS = 8,  // (unit, slots)
  HANDLER      = 9,  // (offset, segment) of the order's handler
  LEVEL_RAISE  = 10, // (slot, value)
  AVERAGE9     = 11, // (unit)
  MOVE         = 12, // (repeat)
};

// The progress an order is given when its hold level runs out.
constexpr std::uint8_t PROGRESS_HELD_OUT{ 200 };
} // namespace

// sp is the stack pointer at entry: x and y, the destination cell, are the frame words at sp-4 and sp-6. The order
// slot and its code are taken before the destination.
void opeu_turn_final_action(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const x{ static_cast<std::uint16_t>(sp - 4) };
  auto const y{ static_cast<std::uint16_t>(sp - 6) };
  auto const slot{ Near(data,
                        Record<Unit>(data, data.current_unit_ptr).orders[0]) };
  auto const code{ Record<Order>(data, slot).kind };
  std::uint16_t const destination[3]{ data.current_unit_ptr, x, y };
  if (!event(ds, DESTINATION, destination, 3)) {
    std::uint16_t const clear[2]{ data.current_unit_ptr, 3 };
    event(ds, CLEAR_ORDERS, clear, 2);
    return;
  }
  std::uint16_t const handler[2]{ data.order_handlers[code].offset,
                                  data.order_handlers[code].segment };
  if (event(ds, HANDLER, handler, 2))
    return;
  std::uint16_t const lower[2]{ slot, 0 };
  if (event(ds, LEVEL_LOWER, lower, 2))
    Record<Order>(data, slot).progress = PROGRESS_HELD_OUT;
  std::uint16_t const unit[1]{ data.current_unit_ptr };
  std::uint16_t const raise[2]{
    slot,
    static_cast<std::uint8_t>(
        static_cast<std::uint16_t>(99 - event(ds, AVERAGE9, unit, 1)) / 35 + 1)
  };
  if (event(ds, LEVEL_RAISE, raise, 2)) {
    std::uint16_t const move[1]{ data.order_moves[code] };
    event(ds, MOVE, move, 1);
  }
}

// The routines behind the action's calls: the unit order destination, the movement levels, clearing orders, the
// order handler (run through the routine link table), the unit's average value9 and the move itself.
uint16_t svc_finalact(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  uint16_t r = 0;
  switch (id) {
    case DESTINATION:
      svc_order_handler(ds, OrderService::UNIT_ORDER_DESTINATION, args, count,
                        &r);
      break;
    case LEVEL_LOWER:
      svc_order_handler(ds, OrderService::MOVE_LEVEL_LOWER, args, count, &r);
      break;
    case CLEAR_ORDERS:
      opeu_clear_unit_orders(ds, Arg(args, count, 0),
                             static_cast<std::uint8_t>(Arg(args, count, 1)));
      break;
    case HANDLER:
      r = static_cast<std::uint16_t>(
          port_call(ds,
                    static_cast<std::uint32_t>(Arg(args, count, 1)) * 16u +
                        Arg(args, count, 0),
                    0, 0));
      break;
    case LEVEL_RAISE:
      r = opeu_move_level_raise(ds, Arg(args, count, 0),
                                static_cast<std::uint8_t>(Arg(args, count, 1)));
      break;
    case AVERAGE9:
      svc_order_handler(ds, OrderService::UNIT_AVERAGE9, args, count, &r);
      break;
    case MOVE:
      svc_order_handler(ds, OrderService::MOVE_UNIT, args, count, &r);
      break;
    default: port_unhandled("finalact", id, count);
  }
  return r;
}
