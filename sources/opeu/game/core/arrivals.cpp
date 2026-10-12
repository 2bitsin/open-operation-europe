// The arrival of reserve units: every unit of a side marked arriving is placed on the map, its division and elements
// made ready, and the human side is shown the arrival.
#include "core/arrivals.hpp"
#include "core/people.hpp"
#include "types.hpp"
#include "units/orders.hpp"

#include "common/arrival.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "map/arrplace.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// Services leave the module through event(ds, id, a0..a3).
enum Service : std::uint16_t {
  LOCATE    = 0, // (&x, &y, side): finds the arrival cell; 0 when there is none
  PREPARE   = 1, // (unit)
  CONTEXT   = 2, // (value)
  LOCATION  = 3, // (x, y, side)
  UNIT_VIEW = 4, // (unit)
  SHOW      = 5, // (id, side, callback offset, callback segment)
  FINISH    = 6, // (value)
};

using ArrivalEvent = auto (*)(std::uint8_t*, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t, std::uint16_t)
    -> std::uint16_t;

constexpr std::uint8_t ORDER_SLOTS{ 3 };
constexpr std::uint8_t RANK_COMMANDER{ 3 };
} // namespace

void opeu_process_arrivals(uint8_t ds[65536], ArrivalEvent event, uint8_t side) {
  auto& data{ Data(ds) };
  data.request_person = opeu_find_side_record(ds, side, RANK_COMMANDER);
  bool arrived{ false };
  for (std::uint8_t g = 0; g < GROUPS_PER_SIDE; ++g) {
    auto& group{ data.groups[GROUPS_PER_SIDE * side + g] };
    for (std::uint8_t u = 0; u < UNITS_PER_GROUP; ++u) {
      auto&      unit{ data.units[UNITS_PER_SIDE * side + UNITS_PER_GROUP * g + u] };
      auto const near{ Near(data, unit) };
      if (!(unit.flags & Bits(UnitFlag::ARRIVING))) continue;
      if (!event(ds, LOCATE, Near(data, unit.x), Near(data, unit.y), side, 0)) return;
      for (auto& task : group.tasks) task.kind = Bits(TaskKind::NONE);
      group.flags &= ~Bits(GroupFlag::RESERVE);
      group.flags |= Bits(GroupFlag::READY);
      unit.flags  &= ~Bits(UnitFlag::ARRIVING);
      unit.flags  |= Bits(UnitFlag::READY);
      opeu_clear_unit_orders(ds, near, ORDER_SLOTS);
      for (auto& element : Elements(data, unit)) {
        element.flags &= ~Bits(ElementFlag::BIT_5);
        element.flags |= Bits(ElementFlag::READY);
      }
      event(ds, PREPARE, near, 0, 0, 0);
      if (data.side_status[side] == SIDE_PLAYED) {
        event(ds, CONTEXT, ARRIVAL_CONTEXT, 0, 0, 0);
        event(ds, LOCATION, unit.x, unit.y, side, 0);
        event(ds, UNIT_VIEW, near, 0, 0, 0);
        data.arrival_side = side;
        data.arrival_unit = near;
        event(ds, SHOW, 0, side, ARRIVAL_PRESENT_OFFSET, ARRIVAL_PRESENT_SEGMENT);
        event(ds, FINISH, 0, 0, 0, 0);
      }
      arrived = true;
    }
    if (arrived) break;
  }
  data.side_flags[side] &= ~Bits(SideFlag::RESERVES_PENDING);
}

// The hourly arrivals: in scenario 3 the second side's reserves arrive only in weather 0 or 1.
void opeu_hourly_c_stage_c(uint8_t ds[65536], ArrivalEvent event) {
  auto& data{ Data(ds) };
  for (std::uint8_t side = 0; side < SIDE_COUNT; ++side)
    if (data.side_flags[side] & Bits(SideFlag::RESERVES_PENDING)) {
      if (CurrentScenario(data) == Scenario::D_DAY && side == 1 && data.weather != 0 && data.weather != 1) continue;
      opeu_process_arrivals(ds, event, side);
    }
}

// The services of the arrivals as direct calls.
uint16_t svc_arrivals(uint8_t* ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3) {
  uint16_t sp, r = 0;
  switch (id) {
  case LOCATE:  r = opeu_arrival_locate(ds, svc_arrplace, a0, a1, (uint8_t)a2); break;
  case PREPARE: // the map routines take the callee's frame pointer (entry SP - 2)
    sp = port_stack_enter(PORT_FRAME);
    opeu_arrival_prepare(ds, svc_maperase, (uint16_t)(sp - 2), a0);
    port_stack_leave(PORT_FRAME);
    break;
  case CONTEXT: opeu_set_turn_context(ds, svc_sound, (uint8_t)a0); break;
  case LOCATION:
    sp = port_stack_enter(PORT_FRAME);
    opeu_arrival_location(ds, svc_mappos, (uint16_t)(sp - 2), (uint8_t)a0, (uint8_t)a1, (uint8_t)a2);
    port_stack_leave(PORT_FRAME);
    break;
  case UNIT_VIEW:
    sp = port_stack_enter(PORT_FRAME);
    opeu_arrival_view(ds, svc_mapunit, (uint16_t)(sp - 2), a0);
    port_stack_leave(PORT_FRAME);
    break;
  case SHOW:
    opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), a0, (uint8_t)a1, (uint32_t)a3 << 16 | a2);
    break;
  case FINISH: opeu_finish_side_initialization(ds, svc_sound, (uint8_t)a0); break;
  default:     port_unhandled("arrivals", id, 4);
  }
  return r;
}
