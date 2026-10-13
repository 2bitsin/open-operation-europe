// The destroy-bridge order: next to the bridge at its destination, the regiment lowers the order's progress by a skill
// roll; when it runs out and no side holds the cell, the bridge is blown (the cell becomes a river crossing again).
#include "units/ordbridg.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "glue.hpp"
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

uint8_t opeu_order_destroy_loss(uint8_t ds[65536], HostEvent event) {
  auto&               data{ Data(ds) };
  auto const          person{ Call(ds, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  std::uint16_t const random{ Call(ds, event, OrderService::RAND_BOUNDED, RollBound(data)) };
  std::uint16_t const average10{ Call(ds, event, OrderService::UNIT_AVERAGE10, data.current_unit_ptr) };
  std::uint16_t const average11{ Call(ds, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(average11 / 19 - static_cast<std::uint16_t>(Record<Person>(data, person).leadership + average10) / 45 + random + 5);
}

void opeu_order_destroy_present(uint8_t ds[65536], HostEvent event) {
  Call(ds, event, OrderService::SOUND_CUE5);
  Call(ds, event, OrderService::PENDING_FINISH, SHOW_DONE);
}

uint16_t opeu_order_destroy_run(uint8_t ds[65536], HostEvent event) {
  auto&             data{ Data(ds) };
  ServiceCell const destination;
  auto const        at_x{ destination.XAt() };
  auto const        at_y{ destination.YAt() };
  Call(ds, event, OrderService::UNIT_ORDER_DESTINATION, data.current_unit_ptr, at_x, at_y);
  if (static_cast<Terrain>(static_cast<std::uint8_t>(Call(ds, event, OrderService::MAP_CODE, Byte(ds, at_x), Byte(ds, at_y)))) != Terrain::BRIDGE || StandsOn(ds, at_x, at_y)) {
    Call(ds, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
    return 1;
  }
  if (!Call(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, at_x), Byte(ds, at_y))) return 0;
  auto const loss{ opeu_order_destroy_loss(ds, event) };
  if (Call(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss) && static_cast<std::uint8_t>(Call(ds, event, OrderService::CELL_SIDE, Byte(ds, at_x), Byte(ds, at_y))) == NO_SIDE) {
    Call(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.order_bridge_destroyed));
    Call(ds, event, OrderService::DAILY_SHOW, BRIDGE_DESTROYED_PICTURE, data.current_side, DESTROY_PRESENT, ORDER_SEGMENT);
    Call(ds, event, OrderService::MAP_MARK5, Byte(ds, at_x), Byte(ds, at_y));
    Call(ds, event, OrderService::MAP_ERASE, Byte(ds, at_x), Byte(ds, at_y));
    Call(ds, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
  }
  return 1;
}

// The destroy-bridge order's calls are order services.
uint16_t svc_ordbridg(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
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
  case OrderService::SOUND_CUE5:             svc_order_handler(ds, id, args, count, &r); break;
  default:                                   port_unhandled("ordbridg", id, count);
  }
  return r;
}

auto port_routine_opeu_order_destroy_present(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_order_destroy_present(ds, svc_ordbridg);
  return 0;
}

auto port_routine_opeu_order_destroy_run(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_destroy_run(ds, svc_ordbridg);
}
