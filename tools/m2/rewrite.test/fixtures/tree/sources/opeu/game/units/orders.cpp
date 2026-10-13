// The fixture's forwarding dispatchers and the Calls that go through them.
#include "units/orders.hpp"
#include "common/memory.hpp"

#include "port.hpp"

#include <array>

void opeu_unit_orders_run(std::uint8_t* ds, std::uint16_t unit) {
  auto const value{ Call(ds, svc_unit_orders, OrderService::UNIT_VALUE, unit) };
  auto const side{ Call(ds, svc_unit_table, SIDE, value) };
  Call(ds, svc_unit_table, REPORT, unit);
  Call(ds, svc_unit_attack, OrderService::WAIT_UNITS, side);
  Call(ds, svc_unit_attack, OrderService::SOUND_CUE);
  Call(ds, svc_unit_panel, OrderService::UNIT_VALUE, unit);
}

// The shared services: whether the id is one of them, and its result through result.
int svc_order_handler(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count, uint16_t* result) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_VALUE: r = opeu_unit_value(ds, Arg(args, count, 0)); break;
  case OrderService::UNIT_SIDE:  r = opeu_unit_side(Arg(args, count, 0)); break;
  case OrderService::WAIT_UNITS: opeu_wait_units(ds, (uint8_t)Arg(args, count, 0)); break;
  default:                       return 0;
  }
  *result = r;
  return 1;
}

// A unit's services: two of the shared ones, handed on as they are.
uint16_t svc_unit_orders(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_VALUE:
  case OrderService::UNIT_SIDE:  svc_order_handler(ds, id, args, count, &r); break;
  case OrderService::SOUND_CUE:  opeu_sound_cue(ds); break;
  default:                       port_unhandled("unit_orders", id, count);
  }
  return r;
}

// The table's services: the shared ones through a table of their order service ids.
uint16_t svc_unit_table(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  constexpr auto                                      NONE{ static_cast<OrderService>(0xff) };
  static constexpr std::array<OrderService, SERVICES> shared{
    OrderService::UNIT_VALUE,
    NONE,
    OrderService::UNIT_SIDE,
    NONE,
  };
  uint16_t r = 0;
  switch (id) {
  case QUANTITY: r = opeu_unit_quantity(ds, Arg(args, count, 0)); break;
  default:
    if (id >= SERVICES || shared[id] == NONE || !svc_order_handler(ds, shared[id], args, count, &r)) port_unhandled("unit_table", id, count);
  }
  return r;
}

// The attack order's services: the shared ones up to UNIT_SIDE, and WAIT_UNITS.
uint16_t svc_unit_attack(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  if ((id > OrderService::UNIT_SIDE && id != OrderService::WAIT_UNITS) || !svc_order_handler(ds, id, args, count, &r)) port_unhandled("unit_attack", id, count);
  return r;
}

// A panel's services: its value is the unit dispatcher's, assigned.
uint16_t svc_unit_panel(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (id) {
  case OrderService::UNIT_VALUE: r = svc_unit_orders(ds, id, args, count); break;
  default:                       break;
  }
  return r;
}
