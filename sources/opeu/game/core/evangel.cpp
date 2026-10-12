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
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
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
constexpr std::uint16_t SHOWN{ 1 }; // DataSegment::side_status of a side the player watches
constexpr std::uint8_t  COMMANDER{ 1 };
constexpr std::uint8_t  OPEN_GROUND{ 1 };
constexpr std::uint8_t  VETERAN{ 70 };
constexpr std::uint16_t MAX_RATING{ 99 };

// The generals of the unit's side whose selected unit it is.
template <typename Body>
auto ForEachGeneralOf(std::uint8_t* ds, std::uint16_t unit, Body&& body) -> void {
  auto&      data{ Data(ds) };
  auto const side{ opeu_unit_side(unit) };
  for (unsigned i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
    auto& person{ data.people[side * PEOPLE_PER_SIDE + i] };
    if (person.status != PERSON_VACANT && opeu_person_selected_unit(ds, Near(data, person)) == unit) body(person);
  }
}

auto Lower(std::uint8_t* ds, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  opeu_sub_byte(static_cast<std::uint16_t>(RandomBelow(ds, spread) + base), &value);
}
auto Raise(std::uint8_t* ds, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  opeu_add_byte(static_cast<std::uint16_t>(RandomBelow(ds, spread) + base), MAX_RATING, &value);
}
} // namespace

uint16_t opeu_angel_unit_eligible(uint8_t ds[65536], HostEvent, uint16_t unit) {
  bool marked{ false };
  ForEachGeneralOf(ds, unit, [&](Person& person) {
    if (person.flags & Bits(PersonRole::POOR_HEALTH)) marked = true;
  });
  if (marked) return 1;
  auto& data{ Data(ds) };
  for (auto const& element : Elements(data, Record<Unit>(data, unit).first))
    if (element.experience >= VETERAN) return 1;
  return 0;
}

// Shows her portrait and one of six remarks; returns the outcome (remark / 2).
uint8_t opeu_angel_portrait(uint8_t ds[65536], HostEvent event, uint8_t) {
  auto const& data{ Data(ds) };
  Call(ds, event, PANEL);
  Call(ds, event, FRAME, 44, 282, 18);
  Call(ds, event, PRINT, NAME);
  Call(ds, event, SEEK, data.portrait_handle, PORTRAIT_LOW, PORTRAIT_HIGH, 0);
  Call(ds, event, DRAW, 45, 316, 8, 80, 0, data.portrait_handle);
  auto const remark{ static_cast<std::uint8_t>(RandomBelow(ds, 6)) };
  Call(ds, event, FORMAT, BUFFER, REMARK_FORMAT, Call(ds, event, LOOKUP, remark + FIRST_REMARK));
  Call(ds, event, POSITION, 54, 324);
  Call(ds, event, TEXT, BUFFER);
  return static_cast<std::uint8_t>(remark >> 1);
}

// Heals the unit's generals (health back to their limit, poor health cleared) and clears its battalions' experience.
void opeu_angel_finish(uint8_t ds[65536], HostEvent, uint16_t, uint16_t unit) {
  ForEachGeneralOf(ds, unit, [](Person& person) {
    person.flags  &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
    person.health  = person.max_health;
  });
  auto& data{ Data(ds) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) element.experience = 0;
}

// Lowers the general's charm by 1..10 and each battalion's morale by 20..49.
void opeu_angel_demoralize(uint8_t ds[65536], HostEvent event, uint16_t person, uint16_t unit) {
  auto& data{ Data(ds) };
  Lower(ds, Record<Person>(data, person).charm, 1, 10);
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Lower(ds, element.morale, 20, 30);
  opeu_angel_finish(ds, event, person, unit);
}

// Raises each battalion's morale by 20..49.
void opeu_angel_inspire(uint8_t ds[65536], HostEvent event, uint16_t person, uint16_t unit) {
  auto& data{ Data(ds) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Raise(ds, element.morale, 20, 30);
  opeu_angel_finish(ds, event, person, unit);
}

// Demoralizes a unit of side 0; raises the general's charm and the battalions' morale on side 1.
void opeu_angel_judge_side(uint8_t ds[65536], HostEvent event, uint16_t person, uint16_t unit) {
  auto&      data{ Data(ds) };
  auto&      charm{ Record<Person>(data, person).charm };
  auto const side_1{ opeu_unit_side(unit) != 0 };
  if (side_1)
    Raise(ds, charm, 1, 10);
  else
    Lower(ds, charm, 1, 10);
  for (auto& element : Elements(data, Record<Unit>(data, unit).first))
    if (side_1)
      Raise(ds, element.morale, 20, 30);
    else
      Lower(ds, element.morale, 20, 30);
  opeu_angel_finish(ds, event, person, unit);
}

// The presentation's far routine: her portrait and remark, then the outcome the remark chose for the commander's unit.
void opeu_angel_present(uint8_t ds[65536], HostEvent event, uint16_t code_segment) {
  auto&      data{ Data(ds) };
  auto const person{ opeu_find_side_record(ds, data.event_side, COMMANDER) };
  auto const unit{ opeu_person_selected_unit(ds, person) };
  auto const action{ data.angel_actions[opeu_angel_portrait(ds, event, data.event_side)] };
  if (action.segment == code_segment && action.offset == DEMORALIZE_OFFSET)
    opeu_angel_demoralize(ds, event, person, unit);
  else if (action.segment == code_segment && action.offset == INSPIRE_OFFSET)
    opeu_angel_inspire(ds, event, person, unit);
  else if (action.segment == code_segment && action.offset == JUDGE_OFFSET)
    opeu_angel_judge_side(ds, event, person, unit);
  else
    Call(ds, event, FAR_ACTION, person, unit);
}

void opeu_angel_event(uint8_t ds[65536], HostEvent event, uint16_t code_segment) {
  auto& data{ Data(ds) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SHOWN) continue;
    auto const  unit{ opeu_find_side_selected_unit(ds, data.event_side) };
    auto const& record{ Record<Unit>(data, unit) };
    auto const  x{ record.x };
    auto const  y{ record.y };
    if (opeu_map_code(ds, x, y) == OPEN_GROUND && RandomBelow(ds, 30) == 3 && opeu_angel_unit_eligible(ds, event, unit)) {
      Call(ds, event, LOCATION, x, y, data.event_side);
      Call(ds, event, MARK, x, y, data.event_side);
      Call(ds, event, SHOW, ANGEL_SHOW, data.event_side, PRESENT_OFFSET, code_segment);
    }
  }
}

uint16_t svc_evangel(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case PANEL: opeu_message_panel_begin(ds, (const DialogHost*)port_dialogs_host()); break;
  case FRAME: opeu_context_frame(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PRINT: opeu_cursor_print(ds, { args, count }); break;
  case SEEK:  result = static_cast<uint16_t>(opeu_dos_seek((int16_t)Arg(args, count, 0), (int32_t)Arg32(args, count, 1), (int16_t)Arg(args, count, 3))); break;
  case DRAW:
    opeu_image_read_draw(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), (int16_t)Arg(args, count, 5));
    break;
  case LOOKUP:   result = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case FORMAT:   opeu_request_format(ds, { args, count }); break;
  case POSITION: opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case TEXT:     opeu_text_feed(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case LOCATION: {
    auto const frame{ port_frame_enter() };
    opeu_arrival_location(ds, svc_mappos, frame, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    port_frame_leave();
    break;
  }
  case MARK: {
    auto const frame{ port_frame_enter() };
    opeu_location_flash(ds, svc_mapunit, frame, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    port_frame_leave();
    break;
  }
  case SHOW: port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case FAR_ACTION:
    port_fatal("evangel: an outcome routine outside this module (person %04x "
               "unit %04x)",
               Arg(args, count, 0), Arg(args, count, 1));
    break;
  default: port_unhandled("evangel", id, count);
  }
  return result;
}

auto port_routine_opeu_angel_event(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_angel_event(ds, svc_evangel, 0x108c);
  return 0;
}

auto port_routine_opeu_angel_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_angel_present(ds, svc_evangel, 0x108c);
  return 0;
}
