// The destroy-bridge order: next to the bridge at its destination, the regiment lowers the order's progress by a skill
// roll; when it runs out and no side holds the cell, the bridge is blown (the cell becomes a river crossing again).
#include "units/ordbridg.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// The day's picture of a blown bridge and the link-table address of the routine that ends its show.
constexpr std::uint16_t BRIDGE_DESTROYED_PICTURE{ 0x5b };
constexpr std::uint16_t ORDER_SEGMENT{ 0x3131 };
constexpr std::uint16_t DESTROY_PRESENT{ 0x060c };
} // namespace

uint8_t opeu_order_destroy_loss(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  auto const          person{ Call(game, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  std::uint16_t const random{ Call(game, event, OrderService::RAND_BOUNDED, RollBound(data)) };
  std::uint16_t const average10{ Call(game, event, OrderService::UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average11{ Call(game, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / 45 + random + 5);
}

void opeu_order_destroy_present(Game& game, HostEvent event) {
  Call(game, event, OrderService::SOUND_CUE5);
  Call(game, event, OrderService::PENDING_FINISH, SHOW_DONE);
}

uint16_t opeu_order_destroy_run(Game& game, HostEvent event) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  Call(game, event, OrderService::UNIT_ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  if (static_cast<Terrain>(static_cast<std::uint8_t>(Call(game, event, OrderService::MAP_CODE, Byte(game, at_x), Byte(game, at_y)))) != Terrain::BRIDGE || StandsOn(game, at_x, at_y)) {
    Call(game, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
    return 1;
  }
  if (!Call(game, event, OrderService::MOVE_ADJACENT, Byte(game, at_x), Byte(game, at_y))) return 0;
  auto const loss{ opeu_order_destroy_loss(game, event) };
  if (Call(game, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss)
      && static_cast<std::uint8_t>(Call(game, event, OrderService::CELL_SIDE, Byte(game, at_x), Byte(game, at_y))) == NO_SIDE) {
    Call(game, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_bridge_destroyed));
    Call(game, event, OrderService::DAILY_SHOW, BRIDGE_DESTROYED_PICTURE, data.current_side, DESTROY_PRESENT, ORDER_SEGMENT);
    Call(game, event, OrderService::MAP_MARK5, Byte(game, at_x), Byte(game, at_y));
    Call(game, event, OrderService::MAP_ERASE, Byte(game, at_x), Byte(game, at_y));
    Call(game, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
  }
  return 1;
}

// The destroy-bridge order's calls are order services.
uint16_t svc_ordbridg(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_AVERAGE11:
  case OrderService::RAND_BOUNDED:
  case OrderService::RESTORE_UNIT_ORDER:
  case OrderService::UNIT_REPORT:
  case OrderService::DAILY_SHOW:
  case OrderService::MOVE_ADJACENT:
  case OrderService::MOVE_LEVEL_LOWER:
  case OrderService::CELL_SIDE:
  case OrderService::UNIT_PERSON:
  case OrderService::UNIT_AVERAGE10:
  case OrderService::UNIT_ORDER_DESTINATION:
  case OrderService::MAP_CODE:
  case OrderService::MAP_MARK5:
  case OrderService::MAP_ERASE:
  case OrderService::PENDING_FINISH:
  case OrderService::SOUND_CUE5:             svc_order_handler(game, id, args, count, &r); break;
  default:                                   port_unhandled("ordbridg", id, count);
  }
  return r;
}

auto port_routine_opeu_order_destroy_present(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_destroy_present(game, svc_ordbridg);
  return 0;
}

auto port_routine_opeu_order_destroy_run(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_destroy_run(game, svc_ordbridg);
}
