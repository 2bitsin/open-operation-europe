// The Air Force command: chooses the air-force officer and his ratings, then lists the pending air strike, airborne
// drop and airlift, asks which mission to plan and runs it, until the side's turn is used up.
#include "core/airforce.hpp"
#include "common/events.hpp"
#include "common/menu.hpp"
#include "core/template.hpp"
#include "types.hpp"
#include "units/labels.hpp"

#include "common/air-service.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/airbomb.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

using enum AirService;

constexpr std::uint8_t  OFFICER_KIND{ 6 };      // the person template of the air-force officer
constexpr std::uint16_t LOCATION_BYTES{ 0x28 }; // the location name buffer of the command, which a format service writes
constexpr std::uint16_t TABLE_X{ 0x2d };
constexpr std::uint16_t TABLE_Y{ 0xb5 };
constexpr std::uint16_t ROW_HEIGHT{ 0x12 };

// The command's event, for the location label's format service (a plain function pointer has no capture).
HostEvent air_event;

auto LabelEvent(Game& game, std::uint16_t, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  return air_event(game, std::to_underlying(FORMAT), args, count);
}

auto Location(Game& game, HostEvent event, std::uint8_t x, std::uint8_t y, std::uint16_t buffer) -> std::uint16_t {
  air_event = event;
  return opeu_location_name(game, LabelEvent, x, y, buffer);
}

auto Average(std::uint8_t a, std::uint8_t b) -> std::uint8_t {
  return static_cast<std::uint8_t>((a + b) / 2);
}
} // namespace

// Picks the air-force officer for the commander's nations; his ratings are the averages of the commander's and the
// chief of staff's leadership, charm and experience.
void opeu_air_officer(Game& game, HostEvent) {
  auto&       data{ Data(game) };
  auto const& commander{ Record<Person>(data, data.context_record1) };
  auto const& staff{ Record<Person>(data, data.context_record3) };
  data.air_person     = opeu_find_person_template(game, commander.status, OFFICER_KIND);
  data.air_leadership = Average(staff.leadership, commander.leadership);
  data.air_charm      = Average(staff.charm, commander.charm);
  data.air_experience = Average(staff.experience, commander.experience);
}

// The mission table header.
void opeu_air_header(Game& game, HostEvent event) {
  Call(game, event, POSITION, 0x38, 0xa3);
  Call(game, event, PRINT, Near(Data(game), Data(game).air_header_format));
}

// The Air Force command loop.
void opeu_turn_action_8(Game& game, HostEvent event) {
  ServiceLocals const locals{ LOCATION_BYTES };
  auto&               data{ Data(game) };
  auto const          buffer{ locals.At(0) };
  opeu_air_officer(game, event);
  do {
    auto const&   side{ Record<Side>(data, data.current_side_ptr) };
    std::uint16_t row{ 0 };
    Call(game, event, TITLE, Near(data, data.air_title));
    if (side.strike.planes) {
      opeu_air_header(game, event);
      Call(game, event, POSITION, TABLE_X, TABLE_Y);
      Call(game, event, PRINT, Near(data, data.air_strike_format), Location(game, event, side.strike.x, side.strike.y, buffer), side.strike.planes, side.strike.hours);
      row = 1;
    }
    if (side.drop.transports) {
      opeu_air_header(game, event);
      Call(game, event, POSITION, TABLE_X, row * ROW_HEIGHT + TABLE_Y);
      Call(game, event, PRINT, Near(data, data.air_drop_format), Location(game, event, side.drop.x, side.drop.y, buffer), side.drop.transports, side.drop.hours);
      ++row;
    }
    if (side.lift.planes) {
      opeu_air_header(game, event);
      Call(game, event, POSITION, TABLE_X, row * ROW_HEIGHT + TABLE_Y);
      Call(game, event, PRINT, Near(data, data.air_lift_format), Location(game, event, side.lift.x, side.lift.y, buffer), side.lift.planes, side.lift.hours);
    }
    auto const choice{ Call(game, event, DIALOG, Near(data, data.air_mission_question), 3, data.context_record2, Near(data, data.air_mission_menu)) };
    if (choice == MENU_CANCELLED) break;
    auto const mission{ data.air_missions[choice - 1] };
    Call(game, event, FAR_ACTION, mission.segment, mission.offset);
    Call(game, event, CONTEXT, data.current_side);
  } while (!data.side_delays[data.current_side]);
}

uint16_t svc_airforce(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_air(game, "airforce", id, args, count);
}

auto port_routine_opeu_turn_action_8(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_turn_action_8(game, svc_airforce);
  return 0;
}
