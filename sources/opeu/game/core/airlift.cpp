// The Airlift command: asks how much ammunition, fuel and food to fly (3000 units a cargo plane, at most 20 planes
// a kind and 60 in all), the drop point and the delay; the air-force officer may stretch or refuse the delay, and
// the staff's confirmation records the order as the current side's pending airlift.
#include "core/airlift.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/airhelp.hpp"
#include "core/dialogs.hpp"
#include "core/limits.hpp"
#include "core/msglook.hpp"
#include "core/random.hpp"
#include "core/textout.hpp"
#include "map/mappick.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/airbomb.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

using enum AirService;

constexpr std::uint16_t LOAD{ 3000 };       // the units one cargo plane carries
constexpr std::uint16_t KIND_PLANES{ 20 };  // the most planes one kind of supply takes
constexpr std::uint16_t TOTAL_PLANES{ 60 }; // the most planes one airlift takes
constexpr std::uint8_t  MAX_LEAD{ 24 };
constexpr std::uint8_t  DUE_AFTER{ 6 }; // the order's countdown runs this many hours past the delay

// Message ids (message_lookup).
constexpr std::uint16_t LEAD_ACCEPTED{ 0x73 }; // the officer accepts the shorter delay (%d)
constexpr std::uint16_t LIFT_REFUSED{ 0x74 };  // the officer refuses a daytime airlift

constexpr std::uint16_t TEXT_BYTES{ 0x66 }; // the message text of the lead and the order, which the format service writes
// The drop point, which the pick service writes: offsets in the command's reserve.
constexpr std::uint16_t CELL_Y_AT{ 0 };
constexpr std::uint16_t CELL_X_AT{ 4 };
constexpr std::uint16_t CELL_BYTES{ 6 };

// The planes `amount` units of one supply kind take: whole loads, a part load counting as one.
constexpr auto PlanesFor(std::uint16_t amount) -> std::uint16_t {
  return static_cast<std::uint16_t>(amount / LOAD + (amount % LOAD > 0 ? 1 : 0));
}
} // namespace

// The officer's preparation lead: min(planes - leadership / 30 + 3, 24) hours, never 1. A shorter delay is answered
// with "at least %d hours" (returning the lead) when 90 - rand(30) beats the officer's skill, else accepted. A delay
// that lands in daytime is refused outright (0) when 60 - rand(20) beats the skill. Returns the delay.
uint8_t opeu_air_lift_lead(Game& game, uint8_t hours, uint16_t planes) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  auto                lead{ static_cast<std::uint8_t>(opeu_min16(static_cast<std::uint16_t>(planes - data.air_leadership / 30 + 3), MAX_LEAD)) };
  if (lead == 1) lead = 2;
  if (hours < lead) {
    if (90 - static_cast<std::uint8_t>(opeu_rand_bounded(30)) > data.air_charm) {
      opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.lift_lead_format)), static_cast<std::uint16_t>(lead) });
      opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.air_person);
      return lead;
    }
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(opeu_message_lookup(game, svc_msglook, static_cast<short>(LEAD_ACCEPTED))),
                                                            static_cast<std::uint16_t>(lead + 1) });
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.air_person);
    return hours;
  }
  if (opeu_hour_is_day(static_cast<std::uint8_t>((hours + data.hour) % 24)) && 60 - static_cast<std::uint8_t>(opeu_rand_bounded(20)) > data.air_charm) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), opeu_message_lookup(game, svc_msglook, static_cast<short>(LIFT_REFUSED)), data.air_person);
    return 0;
  }
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.lift_understood), data.air_person);
  return hours;
}

// Announces "In %d hour(s), %d plane(s) will start their cargo lift" and, if the staff confirms, records the order.
// Returns 1 if recorded.
uint16_t opeu_air_lift_order(Game& game, uint8_t x, uint8_t y, uint8_t hours, uint16_t planes, uint16_t ammo, uint16_t fuel, uint16_t food) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto&               data{ Data(game) };
  auto const          text{ locals.At(0) };
  opeu_request_format(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.lift_start_format)), static_cast<std::uint16_t>(hours),
                                                          static_cast<std::uint16_t>(hours == 1 ? Near(data, data.air_lift_hour) : Near(data, data.air_lift_hours)), static_cast<std::uint16_t>(planes),
                                                          static_cast<std::uint16_t>(planes == 1 ? Near(data, data.air_lift_plane) : Near(data, data.air_lift_planes)) });
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, data.air_person);
  if (!opeu_air_confirm(game)) return 0;
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
void opeu_air_lift_command(Game& game) {
  auto&      data{ Data(game) };
  auto const ask{ [&](std::uint16_t text, std::uint16_t low, std::uint16_t high) { return opeu_air_ask_count(game, text, low, high, 1, data.context_record3, 0); } };
  auto&      side{ Record<Side>(data, data.current_side_ptr) };
  if (side.lift.planes != 0) {
    if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.lift_cancel), data.context_record3)) return;
    side.lift.planes = 0;
  }
  std::uint16_t remaining{ opeu_air_free_planes(game) };
  if (remaining == 0) {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.lift_no_planes), data.context_record3);
    return;
  }
  remaining = opeu_min16(remaining, TOTAL_PLANES);
  auto const initial{ remaining };
  // Asks the amount of one supply kind, bounded by the side's stock and the planes left; 0xffff when cancelled.
  auto const ask_kind{ [&](std::uint16_t text, std::uint16_t stock) {
    auto const most{ opeu_min16(stock, static_cast<std::uint16_t>(opeu_min16(remaining, KIND_PLANES) * LOAD)) };
    auto const amount{ ask(text, 0, most) };
    if (amount != MENU_CANCELLED) remaining -= PlanesFor(amount);
    return amount;
  } };
  auto const ammo{ ask_kind(Near(data, data.lift_ask_ammo), side.stock[0]) };
  if (ammo == MENU_CANCELLED) return;
  auto const fuel{ ask_kind(Near(data, data.lift_ask_fuel), side.stock[1]) };
  if (fuel == MENU_CANCELLED) return;
  auto const food{ ask_kind(Near(data, data.lift_ask_food), side.stock[2]) };
  if (food == MENU_CANCELLED) return;
  if (food + fuel + ammo == 0) return;
  ServiceLocals const locals{ CELL_BYTES };
  auto const          at_x{ locals.At(CELL_X_AT) };
  auto const          at_y{ locals.At(CELL_Y_AT) };
  auto&               cell_x{ Byte(game, at_x) };
  auto&               cell_y{ Byte(game, at_y) };
  cell_x = static_cast<std::uint8_t>(data.viewport_x);
  cell_y = static_cast<std::uint8_t>(data.viewport_y);
  do {
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.lift_select_point), data.context_record3);
    if (!opeu_map_pick_location(game, at_x, at_y)) return;
  } while (!opeu_air_target_valid(game, cell_x, cell_y, Near(data, data.lift_cargo_name)));
  auto const delay{ ask(Near(data, data.lift_ask_hours), 1, 24) };
  if (delay == MENU_CANCELLED) return;
  auto       hours{ static_cast<std::uint8_t>(delay) };
  auto const planes{ static_cast<std::uint16_t>(initial - remaining) };
  if (!opeu_confirm_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.lift_confirm), data.context_record3)) return;
  hours = opeu_air_lift_lead(game, hours, planes);
  if (hours && !opeu_air_lift_order(game, cell_x, cell_y, hours, planes, ammo, fuel, food)) return;
  data.side_delays[data.current_side] = 1;
}

auto port_routine_opeu_air_lift_command(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_air_lift_command(game);
  return 0;
}
