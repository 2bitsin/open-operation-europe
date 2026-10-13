// The Air Force command: chooses the air-force officer and his ratings, then lists the pending air strike, airborne
// drop and airlift, asks which mission to plan and runs it, until the side's turn is used up.
#include "core/airforce.hpp"
#include "common/events.hpp"
#include "core/template.hpp"
#include "types.hpp"
#include "units/labels.hpp"

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

constexpr std::uint8_t  OFFICER_KIND{ 6 }; // the person template of the air-force officer
constexpr std::uint16_t CANCELLED{ 0xffff };
constexpr std::uint16_t LOCATION_BYTES{ 0x28 }; // the location name buffer of the command, which a format service writes
constexpr std::uint16_t COMMAND_FRAME{ 0x34 };  // the SP the command's near calls enter with, below sp
constexpr std::uint16_t TABLE_X{ 0x2d };
constexpr std::uint16_t TABLE_Y{ 0xb5 };
constexpr std::uint16_t ROW_HEIGHT{ 0x12 };

// The command's event, for the location label's format service (a plain function pointer has no capture).
HostEvent air_event;

auto LabelEvent(std::uint8_t* ds, std::uint16_t, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  return air_event(ds, std::to_underlying(FORMAT), args, count);
}

auto Location(std::uint8_t* ds, HostEvent event, std::uint8_t x, std::uint8_t y, std::uint16_t buffer) -> std::uint16_t {
  air_event = event;
  return opeu_location_name(ds, LabelEvent, x, y, buffer);
}

auto Average(std::uint8_t a, std::uint8_t b) -> std::uint8_t {
  return static_cast<std::uint8_t>((a + b) / 2);
}
} // namespace

// Picks the air-force officer for the commander's nations; his ratings are the averages of the commander's and the
// chief of staff's leadership, charm and experience.
void opeu_air_officer(uint8_t ds[65536], HostEvent, uint16_t) {
  auto&       data{ Data(ds) };
  auto const& commander{ Record<Person>(data, data.context_record1) };
  auto const& staff{ Record<Person>(data, data.context_record3) };
  data.air_person     = opeu_find_person_template(ds, commander.status, OFFICER_KIND);
  data.air_leadership = Average(staff.leadership, commander.leadership);
  data.air_charm      = Average(staff.charm, commander.charm);
  data.air_experience = Average(staff.experience, commander.experience);
}

// The mission table header.
void opeu_air_header(uint8_t ds[65536], HostEvent event, uint16_t) {
  Call(ds, event, POSITION, 0x38, 0xa3);
  Call(ds, event, PRINT, Near(Data(ds), Data(ds).air_header_format));
}

// The Air Force command loop.
void opeu_turn_action_8(uint8_t ds[65536], HostEvent event, uint16_t sp) {
  ServiceLocals const locals{ LOCATION_BYTES };
  auto&               data{ Data(ds) };
  auto const          buffer{ locals.At(0) };
  auto const          callee_sp{ static_cast<std::uint16_t>(sp - COMMAND_FRAME) };
  opeu_air_officer(ds, event, callee_sp);
  do {
    auto const&   side{ Record<Side>(data, data.current_side_ptr) };
    std::uint16_t row{ 0 };
    Call(ds, event, TITLE, Near(data, data.air_title));
    if (side.strike.planes) {
      opeu_air_header(ds, event, callee_sp);
      Call(ds, event, POSITION, TABLE_X, TABLE_Y);
      Call(ds, event, PRINT, Near(data, data.air_strike_format), Location(ds, event, side.strike.x, side.strike.y, buffer), side.strike.planes, side.strike.hours);
      row = 1;
    }
    if (side.drop.transports) {
      opeu_air_header(ds, event, callee_sp);
      Call(ds, event, POSITION, TABLE_X, row * ROW_HEIGHT + TABLE_Y);
      Call(ds, event, PRINT, Near(data, data.air_drop_format), Location(ds, event, side.drop.x, side.drop.y, buffer), side.drop.transports, side.drop.hours);
      ++row;
    }
    if (side.lift.planes) {
      opeu_air_header(ds, event, callee_sp);
      Call(ds, event, POSITION, TABLE_X, row * ROW_HEIGHT + TABLE_Y);
      Call(ds, event, PRINT, Near(data, data.air_lift_format), Location(ds, event, side.lift.x, side.lift.y, buffer), side.lift.planes, side.lift.hours);
    }
    auto const choice{ Call(ds, event, DIALOG, Near(data, data.air_mission_question), 3, data.context_record2, Near(data, data.air_mission_menu)) };
    if (choice == CANCELLED) break;
    auto const mission{ data.air_missions[choice - 1] };
    Call(ds, event, FAR_ACTION, mission.segment, mission.offset);
    Call(ds, event, CONTEXT, data.current_side);
  } while (!data.side_delays[data.current_side]);
}

uint16_t svc_airforce(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_air(ds, "airforce", id, args, count);
}

auto port_routine_opeu_turn_action_8(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  auto const sp{ c2_air_sp_enter() };
  opeu_turn_action_8(ds, svc_airforce, sp);
  c2_air_leave();
  return 0;
}
