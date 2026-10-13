// Sister Angelique's visit: each hour a selected unit of a side the player watches, standing on open ground, may be
// visited (1 in 30) when one of its generals is in poor health or one of its battalions has 70 or more experience.
// She appears with a remark and either demoralizes the unit, inspires it, or (by side) does one or the other; every
// outcome then heals the unit's generals and clears its battalions' experience.
#include "core/evangel.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/perssel.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/grid.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the visit calls through its event.
enum Service : std::uint16_t {
  PANEL      = 0,  // () the message panel
  FRAME      = 1,  // (x, y, width)
  PRINT      = 2,  // (format)
  SEEK       = 3,  // (handle, offset low, offset high, origin) in the portrait file
  DRAW       = 4,  // (x, y, width, height, mode, handle) the portrait read from the file
  LOOKUP     = 5,  // (id) -> the message text
  FORMAT     = 6,  // (buffer, format, text)
  POSITION   = 7,  // (x, y)
  TEXT       = 8,  // (buffer)
  LOCATION   = 9,  // (x, y, side) center the map
  MARK       = 10, // (x, y, side) flash the cell
  SHOW       = 11, // (id, side, presenter offset, presenter segment)
  FAR_ACTION = 12, // (person, unit) an outcome routine outside this module
};

constexpr std::uint16_t BUFFER{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t NAME{ offsetof(DataSegment, angel_name) };
constexpr std::uint16_t REMARK_FORMAT{ offsetof(DataSegment, angel_format) };
constexpr std::uint16_t PORTRAIT_LOW{ 0x7c80 }; // her portrait's position in the portrait file
constexpr std::uint16_t PORTRAIT_HIGH{ 3 };
constexpr std::uint16_t FIRST_REMARK{ 37 };
constexpr std::uint16_t ANGEL_SHOW{ 215 };
constexpr std::uint16_t PRESENT_OFFSET{ 0x0ef6 }; // opeu_angel_present as the presentation's far routine
// The outcome routines' offsets in this module's code segment, as angel_actions holds them.
constexpr std::uint16_t DEMORALIZE_OFFSET{ 0x0db8 };
constexpr std::uint16_t INSPIRE_OFFSET{ 0x0e0e };
constexpr std::uint16_t JUDGE_OFFSET{ 0x0e4e };
constexpr std::uint8_t  COMMANDER{ 1 };
constexpr std::uint8_t  OPEN_GROUND{ 1 };
constexpr std::uint8_t  VETERAN{ 70 };

// The generals of the unit's side whose selected unit it is.
template <typename Body>
auto ForEachGeneralOf(Game& game, std::uint16_t unit, Body&& body) -> void {
  auto&      data{ Data(game) };
  auto const side{ opeu_unit_side(unit) };
  for (unsigned i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
    auto& person{ data.people[side * PEOPLE_PER_SIDE + i] };
    if (person.status != PERSON_VACANT && opeu_person_selected_unit(game, Near(data, person)) == unit) body(person);
  }
}

auto Lower(Game& game, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  opeu_sub_byte(static_cast<std::uint16_t>(RandomBelow(game, spread) + base), &value);
}
auto Raise(Game& game, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  opeu_add_byte(static_cast<std::uint16_t>(RandomBelow(game, spread) + base), RATING_LIMIT, &value);
}
} // namespace

uint16_t opeu_angel_unit_eligible(Game& game, HostEvent, uint16_t unit) {
  bool marked{ false };
  ForEachGeneralOf(game, unit, [&](Person& person) {
    if (person.flags & Bits(PersonRole::POOR_HEALTH)) marked = true;
  });
  if (marked) return 1;
  auto& data{ Data(game) };
  for (auto const& element : Elements(data, Record<Unit>(data, unit).first))
    if (element.experience >= VETERAN) return 1;
  return 0;
}

// Shows her portrait and one of six remarks; returns the outcome (remark / 2).
uint8_t opeu_angel_portrait(Game& game, HostEvent event, uint8_t) {
  auto const& data{ Data(game) };
  Call(game, event, PANEL);
  Call(game, event, FRAME, 44, 282, 18);
  Call(game, event, PRINT, NAME);
  Call(game, event, SEEK, data.portrait_handle, PORTRAIT_LOW, PORTRAIT_HIGH, 0);
  Call(game, event, DRAW, 45, 316, 8, 80, 0, data.portrait_handle);
  auto const remark{ static_cast<std::uint8_t>(RandomBelow(game, 6)) };
  Call(game, event, FORMAT, BUFFER, REMARK_FORMAT, Call(game, event, LOOKUP, remark + FIRST_REMARK));
  Call(game, event, POSITION, 54, 324);
  Call(game, event, TEXT, BUFFER);
  return static_cast<std::uint8_t>(remark >> 1);
}

// Heals the unit's generals (health back to their limit, poor health cleared) and clears its battalions' experience.
void opeu_angel_finish(Game& game, HostEvent, uint16_t, uint16_t unit) {
  ForEachGeneralOf(game, unit, [](Person& person) {
    person.flags  &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
    person.health  = person.max_health;
  });
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) element.experience = 0;
}

// Lowers the general's charm by 1..10 and each battalion's morale by 20..49.
void opeu_angel_demoralize(Game& game, HostEvent event, uint16_t person, uint16_t unit) {
  auto& data{ Data(game) };
  Lower(game, Record<Person>(data, person).charm, 1, 10);
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Lower(game, element.morale, 20, 30);
  opeu_angel_finish(game, event, person, unit);
}

// Raises each battalion's morale by 20..49.
void opeu_angel_inspire(Game& game, HostEvent event, uint16_t person, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Raise(game, element.morale, 20, 30);
  opeu_angel_finish(game, event, person, unit);
}

// Demoralizes a unit of side 0; raises the general's charm and the battalions' morale on side 1.
void opeu_angel_judge_side(Game& game, HostEvent event, uint16_t person, uint16_t unit) {
  auto&      data{ Data(game) };
  auto&      charm{ Record<Person>(data, person).charm };
  auto const side_1{ opeu_unit_side(unit) != 0 };
  if (side_1)
    Raise(game, charm, 1, 10);
  else
    Lower(game, charm, 1, 10);
  for (auto& element : Elements(data, Record<Unit>(data, unit).first))
    if (side_1)
      Raise(game, element.morale, 20, 30);
    else
      Lower(game, element.morale, 20, 30);
  opeu_angel_finish(game, event, person, unit);
}

// The presentation's far routine: her portrait and remark, then the outcome the remark chose for the commander's unit.
void opeu_angel_present(Game& game, HostEvent event, uint16_t code_segment) {
  auto&      data{ Data(game) };
  auto const person{ opeu_find_side_record(game, data.event_side, COMMANDER) };
  auto const unit{ opeu_person_selected_unit(game, person) };
  auto const action{ data.angel_actions[opeu_angel_portrait(game, event, data.event_side)] };
  if (action.segment == code_segment && action.offset == DEMORALIZE_OFFSET)
    opeu_angel_demoralize(game, event, person, unit);
  else if (action.segment == code_segment && action.offset == INSPIRE_OFFSET)
    opeu_angel_inspire(game, event, person, unit);
  else if (action.segment == code_segment && action.offset == JUDGE_OFFSET)
    opeu_angel_judge_side(game, event, person, unit);
  else
    Call(game, event, FAR_ACTION, person, unit);
}

void opeu_angel_event(Game& game, HostEvent event, uint16_t code_segment) {
  auto& data{ Data(game) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SIDE_PLAYED) continue;
    auto const  unit{ opeu_find_side_selected_unit(game, data.event_side) };
    auto const& record{ Record<Unit>(data, unit) };
    auto const  x{ record.x };
    auto const  y{ record.y };
    if (opeu_map_code(game, x, y) == OPEN_GROUND && RandomBelow(game, 30) == 3 && opeu_angel_unit_eligible(game, event, unit)) {
      Call(game, event, LOCATION, x, y, data.event_side);
      Call(game, event, MARK, x, y, data.event_side);
      Call(game, event, SHOW, ANGEL_SHOW, data.event_side, PRESENT_OFFSET, code_segment);
    }
  }
}

uint16_t svc_evangel(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case PANEL: opeu_message_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case FRAME: opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PRINT: opeu_cursor_print(game, { args, count }); break;
  case SEEK:  result = static_cast<uint16_t>(opeu_dos_seek((int16_t)Arg(args, count, 0), (int32_t)Arg32(args, count, 1), (int16_t)Arg(args, count, 3))); break;
  case DRAW:
    opeu_image_read_draw(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4),
                         (int16_t)Arg(args, count, 5));
    break;
  case LOOKUP:   result = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case FORMAT:   opeu_request_format(game, { args, count }); break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TEXT:     opeu_text_feed(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case LOCATION: {
    opeu_arrival_location(game, svc_mappos, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    break;
  }
  case MARK: {
    opeu_location_flash(game, svc_mapunit, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    break;
  }
  case SHOW: port_daily_show(game, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case FAR_ACTION:
    port_fatal("evangel: an outcome routine outside this module (person %04x "
               "unit %04x)",
               Arg(args, count, 0), Arg(args, count, 1));
    break;
  default: port_unhandled("evangel", id, count);
  }
  return result;
}

auto port_routine_opeu_angel_event(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_angel_event(game, svc_evangel, 0x108c);
  return 0;
}

auto port_routine_opeu_angel_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_angel_present(game, svc_evangel, 0x108c);
  return 0;
}
