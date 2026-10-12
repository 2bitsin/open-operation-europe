// The commander's visit: in clear weather a side the player watches may be visited (1 in 70 each hour) when its
// general is fit (health 30 or more, not in poor health) and his unit is supported, safe from enemy special units
// and on open ground of the side's influence. A visitor chats with him and his ratings rise.
#include "core/evvisit.hpp"
#include "common/events.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/perssel.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the event calls through its event.
enum Service : std::uint16_t {
  LOOKUP  = 0, // (id) -> the message text
  FORMAT  = 1, // (buffer, format, value)
  MESSAGE = 2, // (text, person)
  DAILY   = 3, // (side) the commander panel
  CONTEXT = 4, // (value) the turn context
  MARK    = 5, // (x, y, side) flash the unit's cell
  SHOW    = 6, // (id, side, presenter offset, presenter segment)
  FINISH  = 7, // (value) the side's initialization
};

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t VISIT_FORMAT{ offsetof(DataSegment, visit_format) }; // "At ease, Commander %s."
constexpr std::uint16_t VISIT_SHOW{ 0x88 };
constexpr std::uint16_t VISIT_CONTEXT{ 15 };
constexpr std::uint16_t PRESENT_OFFSET{ 0x02ac }; // opeu_visit_present as the presentation's far routine
constexpr std::uint16_t PRESENT_SEGMENT{ 0x108c };
constexpr std::uint16_t SHOWN{ 1 };       // DataSegment::side_status of a side the player watches
constexpr std::uint8_t  COMMANDER{ 1 };   // the side record kind of the commander
constexpr std::uint8_t  VISITOR{ 4 };     // the person template kind of the visitor
constexpr std::uint8_t  OPEN_GROUND{ 1 }; // the map code of a cell the visit happens on
constexpr std::uint8_t  NO_SIDE{ 0xff };
constexpr std::uint8_t  DIRECTIONS{ 8 };
constexpr std::uint8_t  MIN_HEALTH{ 30 };
constexpr std::uint16_t MAX_RATING{ 99 };

auto Raise(std::uint8_t* ds, std::uint8_t& rating) -> void {
  opeu_add_byte(RandomBelow(ds, 10), MAX_RATING, &rating);
}
} // namespace

// Whether no enemy special unit stands next to the unit and its cell is open ground outside the enemy's influence.
uint16_t opeu_visit_unit_safe(uint8_t ds[65536], uint16_t unit) {
  auto const& record{ Record<Unit>(Data(ds), unit) };
  auto const  enemy{ opeu_opposite_side(opeu_unit_side(unit)) };
  for (std::uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
    std::uint8_t x{ record.x };
    std::uint8_t y{ record.y };
    if (opeu_grid_step(&x, &y, direction) && opeu_cell_side(ds, x, y) == enemy
        && opeu_unit_special(ds, opeu_find_unit_at(ds, x, y)))
      return 0;
  }
  return opeu_map_code(ds, record.x, record.y) == OPEN_GROUND
                 && opeu_cell_influence_side(ds, record.x, record.y) != enemy
             ? 1
             : 0;
}

// Raises leadership, charm, bravery, infantry, artillery and armor by 0..9 each, up to 99.
void opeu_visit_person_boost(uint8_t ds[65536], uint16_t person) {
  auto& general{ Record<Person>(Data(ds), person) };
  Raise(ds, general.leadership);
  Raise(ds, general.charm);
  Raise(ds, general.bravery);
  Raise(ds, general.infantry);
  Raise(ds, general.artillery);
  Raise(ds, general.armor);
}

// One of three conversations with the visitor of the commander's nation, then the commander's ratings rise.
void opeu_visit_conversation(uint8_t ds[65536], HostEvent event, uint8_t side) {
  auto const commander{ opeu_find_side_record(ds, side, COMMANDER) };
  auto const speaker{ opeu_find_person_template(
      ds, Record<Person>(Data(ds), opeu_find_side_record(ds, side, COMMANDER)).status, VISITOR) };
  auto const say{ [&](std::uint16_t text) { Call(ds, event, MESSAGE, text, speaker); } };
  auto const lookup{ [&](std::uint16_t id) -> std::uint16_t { return Call(ds, event, LOOKUP, id); } };
  switch (RandomBelow(ds, 3)) {
  case 0:
    Call(ds, event, FORMAT, TEXT, VISIT_FORMAT, commander);
    say(TEXT);
    for (std::uint16_t id{ 12 }; id <= 16; ++id) say(lookup(id));
    break;
  case 1:
    say(lookup(17));
    Call(ds, event, FORMAT, TEXT, lookup(18), commander);
    say(TEXT);
    say(lookup(19));
    say(lookup(20));
    say(lookup(22));
    break;
  case 2:
    Call(ds, event, FORMAT, TEXT, lookup(23), commander);
    say(TEXT);
    for (std::uint16_t id{ 24 }; id <= 26; ++id) say(lookup(id));
    break;
  }
  opeu_visit_person_boost(ds, commander);
}

// The presentation's far routine: the conversation, then the commander panel shown afresh.
void opeu_visit_present(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  opeu_visit_conversation(ds, event, data.event_side);
  data.context_side = NO_SIDE;
  Call(ds, event, DAILY, data.event_side);
}

// Once a day per side (SideFlag::VISITED).
void opeu_visit_event(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SHOWN || (data.side_flags[data.event_side] & Bits(SideFlag::VISITED)))
      continue;
    auto const  record{ opeu_find_side_record(ds, data.event_side, COMMANDER) };
    auto const& commander{ Record<Person>(data, record) };
    if (commander.health < MIN_HEALTH || (commander.flags & Bits(PersonRole::POOR_HEALTH))) continue;
    auto const unit{ opeu_person_selected_unit(ds, record) };
    if (!data.weather && opeu_unit_support_condition(ds, unit) && opeu_visit_unit_safe(ds, unit)
        && RandomBelow(ds, 70) == 1) {
      auto const& target{ Record<Unit>(data, unit) };
      auto const  x{ target.x };
      auto const  y{ target.y };
      Call(ds, event, CONTEXT, VISIT_CONTEXT);
      Call(ds, event, MARK, x, y, data.event_side);
      Call(ds, event, SHOW, VISIT_SHOW, data.event_side, PRESENT_OFFSET, PRESENT_SEGMENT);
      data.side_flags[data.event_side] |= Bits(SideFlag::VISITED);
      Call(ds, event, FINISH, 0);
    }
  }
}

uint16_t svc_evvisit(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case LOOKUP: result = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case FORMAT: c1_request_format(ds, args, count); break;
  case MESSAGE:
    opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1));
    break;
  case DAILY:   opeu_daily_context(ds, svc_context, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case CONTEXT: opeu_set_turn_context(ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case MARK:    {
    auto const frame{ port_frame_enter() };
    opeu_location_flash(ds, svc_mapunit, frame, static_cast<std::uint8_t>(Arg(args, count, 0)),
                        static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    port_frame_leave();
    break;
  }
  case SHOW:
    port_daily_show(ds, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3));
    break;
  case FINISH: opeu_finish_side_initialization(ds, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  default:     port_unhandled("evvisit", id, count);
  }
  return result;
}

auto port_routine_opeu_visit_event(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_visit_event(ds, svc_evvisit);
  return 0;
}

auto port_routine_opeu_visit_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_visit_present(ds, svc_evvisit);
  return 0;
}
