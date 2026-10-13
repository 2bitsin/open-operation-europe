// The defend order: while the regiment stands on its destination, the order's progress falls by a loss that grows
// with the regiment's average value11 and shrinks with the general's leadership and the average value10.
#include "units/orddefnd.hpp"
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

namespace { } // namespace

uint8_t opeu_order_defend_loss(Game& game, HostEvent event) {
  auto&               data{ Data(game) };
  auto const          person{ Call(game, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  std::uint16_t const average10{ Call(game, event, OrderService::UNIT_AVERAGE10, data.current_unit_ptr) };
  auto const          leadership{ static_cast<std::uint16_t>(average10 + Record<Person>(data, person).leadership) / 55 };
  return static_cast<std::uint8_t>(Call(game, event, OrderService::UNIT_AVERAGE11, data.current_unit_ptr) / 30 - leadership + 4);
}

// Nonzero (the order step is done) while the regiment holds its destination.
uint16_t opeu_order_defend_run(Game& game, HostEvent event) {
  auto&             data{ Data(game) };
  ServiceCell const destination;
  auto const        x{ destination.XAt() };
  auto const        y{ destination.YAt() };
  Call(game, event, OrderService::UNIT_ORDER_DESTINATION, data.current_unit_ptr, x, y);
  if (!StandsOn(game, x, y)) return 0;
  auto const loss{ opeu_order_defend_loss(game, event) };
  Call(game, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), loss);
  return 1;
}

// The defend order's calls are order services.
uint16_t svc_orddefnd(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_AVERAGE11:
  case OrderService::MOVE_LEVEL_LOWER:
  case OrderService::UNIT_PERSON:
  case OrderService::UNIT_AVERAGE10:
  case OrderService::UNIT_ORDER_DESTINATION: svc_order_handler(game, id, args, count, &r); break;
  default:                                   port_unhandled("orddefnd", id, count);
  }
  return r;
}

auto port_routine_opeu_order_defend_run(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return opeu_order_defend_run(game, svc_orddefnd);
}
