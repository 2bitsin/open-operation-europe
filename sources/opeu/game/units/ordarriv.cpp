// The move, pursue and rest order handlers and the hold delay. A handler returns 1 when the current regiment's order
// step is finished. The destination and target services write the cell into the frame bytes x and y.
#include "units/ordarriv.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "types.hpp"
#include "units/order-handler.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/engineer.hpp"

using namespace opeu::game;

namespace {
// The bound of the draw an element's experience is held against when a resting regiment resumes its saved order.
constexpr std::uint16_t RESUME_DRAW{ 15 };
} // namespace

// The move order: at the destination, the regiment reports its arrival and its saved order resumes. sp is the entry
// stack pointer; x and y lie at sp-4 and sp-6.
uint16_t opeu_order_arrive(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const x{ static_cast<std::uint16_t>(sp - 4) };
  auto const y{ static_cast<std::uint16_t>(sp - 6) };
  Serve(ds, event, OrderService::UNIT_ORDER_DESTINATION, data.current_unit_ptr, x, y);
  if (!StandsOn(ds, x, y)) return 0;
  Serve(ds, event, OrderService::UNIT_REPORT, data.current_unit_ptr, Near(data, data.arrived_text));
  Serve(ds, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
  return 1;
}

// The pursue order: finished beside a valid target regiment; a lost target restores the saved order. x and y lie at
// sp-6 and sp-8.
uint16_t opeu_order_reach_unit(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto const x{ static_cast<std::uint16_t>(sp - 6) };
  auto const y{ static_cast<std::uint16_t>(sp - 8) };
  if (Serve(ds, event, OrderService::MOVE_UNIT_TARGET, x, y))
    return Serve(ds, event, OrderService::MOVE_ADJACENT, Byte(ds, x), Byte(ds, y)) ? 1 : 0;
  Serve(ds, event, OrderService::RESTORE_UNIT_ORDER, Data(ds).current_unit_ptr);
  return 1;
}

// The hold delay: (99 - the general's charm) / 30 + 1, in unsigned 16 bits.
uint8_t opeu_order_hold_delay(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const person{ Serve(ds, event, OrderService::UNIT_PERSON, data.current_unit_ptr) };
  return static_cast<std::uint8_t>(static_cast<std::uint16_t>(99 - Record<Person>(data, person).charm) / 30 + 1);
}

// The rest order: hold at the destination. Once the hold runs out and an order is saved, the first element whose
// experience is below a draw of 0..14 resumes that order. x and y lie at sp-6 and sp-8.
uint16_t opeu_order_hold(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const x{ static_cast<std::uint16_t>(sp - 6) };
  auto const y{ static_cast<std::uint16_t>(sp - 8) };
  Serve(ds, event, OrderService::UNIT_ORDER_DESTINATION, data.current_unit_ptr, x, y);
  if (!StandsOn(ds, x, y)) return 0;
  // The delay is taken before the current regiment is read again.
  auto const delay{ opeu_order_hold_delay(ds, event) };
  if (Serve(ds, event, OrderService::MOVE_LEVEL_LOWER, CurrentOrder(data), delay)
      && CurrentUnit(data).orders[1].kind != ORDER_UNUSED)
    for (auto const& element : Elements(data, CurrentUnit(data)))
      // The draw comes before the element is read.
      if (auto const draw{ Serve(ds, event, OrderService::RAND_BOUNDED, RESUME_DRAW) }; element.experience < draw) {
        Serve(ds, event, OrderService::RESTORE_UNIT_ORDER, data.current_unit_ptr);
        break;
      }
  return 1;
}

// The handlers' calls are order services.
uint16_t svc_ordarriv(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case OrderService::UNIT_ORDER_DESTINATION:
  case OrderService::UNIT_REPORT:
  case OrderService::RESTORE_UNIT_ORDER:
  case OrderService::MOVE_UNIT_TARGET:
  case OrderService::MOVE_ADJACENT:
  case OrderService::UNIT_PERSON:
  case OrderService::MOVE_LEVEL_LOWER:
  case OrderService::RAND_BOUNDED:           svc_order_handler(ds, id, args, count, &r); break;
  default:                                   port_unhandled("ordarriv", id, count);
  }
  return r;
}

auto port_routine_opeu_order_arrive(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) { return opeu_order_arrive(ds, svc_ordarriv, sp); });
}

auto port_routine_opeu_order_hold(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) { return opeu_order_hold(ds, svc_ordarriv, sp); });
}

auto port_routine_opeu_order_reach_unit(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) { return opeu_order_reach_unit(ds, svc_ordarriv, sp); });
}
