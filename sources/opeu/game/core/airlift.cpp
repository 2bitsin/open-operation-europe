// The Airlift command: asks how much ammunition, fuel and food to fly (3000 units a cargo plane, at most 20 planes
// a kind and 60 in all), the drop point and the delay; the air-force officer may stretch or refuse the delay, and
// the staff's confirmation records the order as the current side's pending airlift.
//
// Routine frames live on the DS stack: sp is the entry SP, a local whose address reaches a callee sits at
// Local(sp, 2 + n), and a call to another air routine passes the SP that call leaves.
#include "types.hpp"
#include "core/airlift.hpp"
#include "core/airhelp.hpp"
#include "core/limits.hpp"
#include "core/random.hpp"
#include "units/value.hpp"
#include "common/events.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/airbomb.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

using enum AirService;

constexpr std::uint16_t LOAD{ 3000 }; // the units one cargo plane carries
constexpr std::uint16_t KIND_PLANES{
  20
}; // the most planes one kind of supply takes
constexpr std::uint16_t TOTAL_PLANES{ 60 }; // the most planes one airlift takes
constexpr std::uint16_t CANCELLED{ 0xffff };
constexpr std::uint8_t  MAX_LEAD{ 24 };
constexpr std::uint8_t  DUE_AFTER{
  6
}; // the order's countdown runs this many hours past the delay

// Message ids (message_lookup).
constexpr std::uint16_t LEAD_ACCEPTED{
  0x73
}; // the officer accepts the shorter delay (%d)
constexpr std::uint16_t LIFT_REFUSED{
  0x74
}; // the officer refuses a daytime airlift

constexpr std::uint16_t TEXT_LOCAL{
  2 + 0x66
}; // the 100-byte message text of the lead and the order
constexpr std::uint16_t CELL_X_LOCAL{ 2 + 0x0c };
constexpr std::uint16_t CELL_Y_LOCAL{ 2 + 0x10 };
constexpr std::uint16_t COMMAND_FRAME{
  0x18
}; // the command's frame: callees enter below it

// The planes `amount` units of one supply kind take: whole loads, a part load counting as one.
constexpr auto PlanesFor(std::uint16_t amount) -> std::uint16_t {
  return static_cast<std::uint16_t>(amount / LOAD +
                                    (amount % LOAD > 0 ? 1 : 0));
}
} // namespace

// The officer's preparation lead: min(planes - leadership / 30 + 3, 24) hours, never 1. A shorter delay is answered
// with "at least %d hours" (returning the lead) when 90 - rand(30) beats the officer's skill, else accepted. A delay
// that lands in daytime is refused outright (0) when 60 - rand(20) beats the skill. Returns the delay.
uint8_t opeu_air_lift_lead(uint8_t ds[65536], HostEvent event, uint16_t sp,
                           uint8_t hours, uint16_t planes) {
  auto&      data{ Data(ds) };
  auto const text{ Local(sp, TEXT_LOCAL) };
  auto       lead{ static_cast<std::uint8_t>(
      opeu_min16(static_cast<std::uint16_t>(planes - data.air_stat_13 / 30 + 3),
                 MAX_LEAD)) };
  if (lead == 1)
    lead = 2;
  if (hours < lead) {
    if (90 - static_cast<std::uint8_t>(opeu_rand_bounded(30)) >
        data.air_stat_14) {
      Call(ds, event, FORMAT, text, Near(data, data.lift_lead_format), lead);
      Call(ds, event, MESSAGE, text, data.air_person);
      return lead;
    }
    Call(ds, event, FORMAT, text, Call(ds, event, LOOKUP, LEAD_ACCEPTED),
         lead + 1);
    Call(ds, event, MESSAGE, text, data.air_person);
    return hours;
  }
  if (opeu_hour_is_day(static_cast<std::uint8_t>((hours + data.hour) % 24)) &&
      60 - static_cast<std::uint8_t>(opeu_rand_bounded(20)) >
          data.air_stat_14) {
    Call(ds, event, MESSAGE, Call(ds, event, LOOKUP, LIFT_REFUSED),
         data.air_person);
    return 0;
  }
  Call(ds, event, MESSAGE, Near(data, data.lift_understood), data.air_person);
  return hours;
}

// Announces "In %d hour(s), %d plane(s) will start their cargo lift" and, if the staff confirms, records the order.
// Returns 1 if recorded.
uint16_t opeu_air_lift_order(uint8_t ds[65536], HostEvent event, uint16_t sp,
                             uint8_t x, uint8_t y, uint8_t hours,
                             uint16_t planes, uint16_t ammo, uint16_t fuel,
                             uint16_t food) {
  auto&      data{ Data(ds) };
  auto const text{ Local(sp, TEXT_LOCAL) };
  Call(ds, event, FORMAT, text, Near(data, data.lift_start_format), hours,
       hours == 1 ? Near(data, data.air_lift_hour)
                  : Near(data, data.air_lift_hours),
       planes,
       planes == 1 ? Near(data, data.air_lift_plane)
                   : Near(data, data.air_lift_planes));
  Call(ds, event, MESSAGE, text, data.air_person);
  if (!opeu_air_confirm(ds, event,
                        static_cast<std::uint16_t>(sp - 2 - 0x6c - 6 - 4)))
    return 0;
  auto& lift{ Record<Side>(data, data.current_side_ptr).lift };
  lift.x           = x;
  lift.y           = y;
  lift.supplies[0] = ammo;
  lift.supplies[1] = fuel;
  lift.supplies[2] = food;
  lift.planes      = planes;
  lift.hours       = hours;
  lift.turns       = static_cast<std::uint8_t>(hours + DUE_AFTER);
  return 1;
}

// The Airlift command. Cancels a pending airlift on request, then asks the cargo amounts, the drop point and the
// delay; uses up the side's turn unless cancelled or refused.
void opeu_air_lift_command(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const callee_sp{ static_cast<std::uint16_t>(sp - COMMAND_FRAME - 4) };
  auto const ask{ [&](std::uint16_t text, std::uint16_t low,
                      std::uint16_t high) {
    return opeu_air_ask_count(
        ds, event, static_cast<std::uint16_t>(sp - COMMAND_FRAME - 6 - 4), text,
        low, high, 1, data.context_record3, 0);
  } };
  auto& side{ Record<Side>(data, data.current_side_ptr) };
  if (side.lift.planes != 0) {
    if (!Call(ds, event, CONFIRM, Near(data, data.lift_cancel),
              data.context_record3))
      return;
    side.lift.planes = 0;
  }
  std::uint16_t remaining{ opeu_air_free_planes(ds, event, callee_sp) };
  if (remaining == 0) {
    Call(ds, event, MESSAGE, Near(data, data.lift_no_planes),
         data.context_record3);
    return;
  }
  remaining = opeu_min16(remaining, TOTAL_PLANES);
  auto const initial{ remaining };
  // Asks the amount of one supply kind, bounded by the side's stock and the planes left; 0xffff when cancelled.
  auto const ask_kind{ [&](std::uint16_t text, std::uint16_t stock) {
    auto const most{ opeu_min16(
        stock, static_cast<std::uint16_t>(opeu_min16(remaining, KIND_PLANES) *
                                          LOAD)) };
    auto const amount{ ask(text, 0, most) };
    if (amount != CANCELLED)
      remaining -= PlanesFor(amount);
    return amount;
  } };
  auto const ammo{ ask_kind(Near(data, data.lift_ask_ammo), side.stock[0]) };
  if (ammo == CANCELLED)
    return;
  auto const fuel{ ask_kind(Near(data, data.lift_ask_fuel), side.stock[1]) };
  if (fuel == CANCELLED)
    return;
  auto const food{ ask_kind(Near(data, data.lift_ask_food), side.stock[2]) };
  if (food == CANCELLED)
    return;
  if (food + fuel + ammo == 0)
    return;
  auto const at_x{ Local(sp, CELL_X_LOCAL) };
  auto const at_y{ Local(sp, CELL_Y_LOCAL) };
  auto&      cell_x{ Byte(ds, at_x) };
  auto&      cell_y{ Byte(ds, at_y) };
  cell_x = static_cast<std::uint8_t>(data.viewport_x);
  cell_y = static_cast<std::uint8_t>(data.viewport_y);
  do {
    Call(ds, event, MESSAGE, Near(data, data.lift_select_point),
         data.context_record3);
    if (!Call(ds, event, PICK, at_x, at_y))
      return;
  } while (!opeu_air_target_valid(ds, event, callee_sp, cell_x, cell_y,
                                  Near(data, data.lift_cargo_name)));
  auto const delay{ ask(Near(data, data.lift_ask_hours), 1, 24) };
  if (delay == CANCELLED)
    return;
  auto       hours{ static_cast<std::uint8_t>(delay) };
  auto const planes{ static_cast<std::uint16_t>(initial - remaining) };
  if (!Call(ds, event, CONFIRM, Near(data, data.lift_confirm),
            data.context_record3))
    return;
  hours = opeu_air_lift_lead(ds, event, callee_sp, hours, planes);
  if (hours &&
      !opeu_air_lift_order(
          ds, event, static_cast<std::uint16_t>(sp - COMMAND_FRAME - 8 - 4),
          cell_x, cell_y, hours, planes, ammo, fuel, food))
    return;
  data.side_delays[data.current_side] = 1;
}

uint16_t svc_airlift(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  return svc_air(ds, "airlift", id, args, count);
}

auto port_routine_opeu_air_lift_command(std::uint8_t* ds, std::uint16_t const*,
                                        unsigned) -> std::uint32_t {
  auto const sp{ c2_air_sp_enter() };
  opeu_air_lift_command(ds, svc_airlift, sp);
  c2_air_leave();
  return 0;
}
