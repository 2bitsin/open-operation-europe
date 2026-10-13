// The commander's visit: in clear weather a side the player watches may be visited (1 in 70 each hour) when its
// general is fit (health 30 or more, not in poor health) and his unit is supported, safe from enemy special units
// and on open ground of the side's influence. A visitor chats with him and his ratings rise.
#include "core/evvisit.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/perssel.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "core/textout.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t VISIT_FORMAT{ offsetof(DataSegment, visit_format) }; // "At ease, Commander %s."
constexpr std::uint16_t VISIT_SHOW{ 0x88 };
constexpr std::uint16_t VISIT_CONTEXT{ 15 };
constexpr std::uint16_t PRESENT_OFFSET{ 0x02ac }; // opeu_visit_present as the presentation's far routine
constexpr std::uint16_t PRESENT_SEGMENT{ 0x108c };
constexpr std::uint8_t  COMMANDER{ 1 };   // the side record kind of the commander
constexpr std::uint8_t  VISITOR{ 4 };     // the person template kind of the visitor
constexpr std::uint8_t  OPEN_GROUND{ 1 }; // the map code of a cell the visit happens on
constexpr std::uint8_t  MIN_HEALTH{ 30 };

auto Raise(Game& game, std::uint8_t& rating) -> void {
  opeu_add_byte(RandomBelow(game, 10), RATING_LIMIT, &rating);
}
} // namespace

// Whether no enemy special unit stands next to the unit and its cell is open ground outside the enemy's influence.
uint16_t opeu_visit_unit_safe(Game& game, uint16_t unit) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  auto const  enemy{ opeu_opposite_side(opeu_unit_side(unit)) };
  for (std::uint8_t direction{ 0 }; direction < GRID_DIRECTIONS; ++direction) {
    std::uint8_t x{ record.x };
    std::uint8_t y{ record.y };
    if (opeu_grid_step(&x, &y, direction) && opeu_cell_side(game, x, y) == enemy && opeu_unit_special(game, opeu_find_unit_at(game, x, y))) return 0;
  }
  return opeu_map_code(game, record.x, record.y) == OPEN_GROUND && opeu_cell_influence_side(game, record.x, record.y) != enemy ? 1 : 0;
}

// Raises leadership, charm, bravery, infantry, artillery and armor by 0..9 each, up to 99.
void opeu_visit_person_boost(Game& game, uint16_t person) {
  auto& general{ Record<Person>(Data(game), person) };
  Raise(game, general.leadership);
  Raise(game, general.charm);
  Raise(game, general.bravery);
  Raise(game, general.infantry);
  Raise(game, general.artillery);
  Raise(game, general.armor);
}

// One of three conversations with the visitor of the commander's nation, then the commander's ratings rise.
void opeu_visit_conversation(Game& game, uint8_t side) {
  auto const commander{ opeu_find_side_record(game, side, COMMANDER) };
  auto const speaker{ opeu_find_person_template(game, Record<Person>(Data(game), opeu_find_side_record(game, side, COMMANDER)).status, VISITOR) };
  auto const say{ [&](std::uint16_t text) { opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, speaker); } };
  auto const lookup{ [&](std::uint16_t id) -> std::uint16_t { return opeu_message_lookup(game, svc_msglook, static_cast<int16_t>(id)); } };
  switch (RandomBelow(game, 3)) {
  case 0:
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(VISIT_FORMAT), static_cast<std::uint16_t>(commander) });
    say(TEXT);
    for (std::uint16_t id{ 12 }; id <= 16; ++id) say(lookup(id));
    break;
  case 1:
    say(lookup(17));
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(lookup(18)), static_cast<std::uint16_t>(commander) });
    say(TEXT);
    say(lookup(19));
    say(lookup(20));
    say(lookup(22));
    break;
  case 2:
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(lookup(23)), static_cast<std::uint16_t>(commander) });
    say(TEXT);
    for (std::uint16_t id{ 24 }; id <= 26; ++id) say(lookup(id));
    break;
  }
  opeu_visit_person_boost(game, commander);
}

// The presentation's far routine: the conversation, then the commander panel shown afresh.
void opeu_visit_present(Game& game) {
  auto& data{ Data(game) };
  opeu_visit_conversation(game, data.event_side);
  data.context_side = NO_SIDE;
  opeu_daily_context(game, static_cast<std::uint8_t>(data.event_side));
}

// Once a day per side (SideFlag::VISITED).
void opeu_visit_event(Game& game) {
  auto& data{ Data(game) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SIDE_PLAYED || (data.side_flags[data.event_side] & Bits(SideFlag::VISITED))) continue;
    auto const  record{ opeu_find_side_record(game, data.event_side, COMMANDER) };
    auto const& commander{ Record<Person>(data, record) };
    if (commander.health < MIN_HEALTH || (commander.flags & Bits(PersonRole::POOR_HEALTH))) continue;
    auto const unit{ opeu_person_selected_unit(game, record) };
    if (!data.weather && opeu_unit_support_condition(game, unit) && opeu_visit_unit_safe(game, unit) && RandomBelow(game, 70) == 1) {
      auto const& target{ Record<Unit>(data, unit) };
      auto const  x{ target.x };
      auto const  y{ target.y };
      opeu_set_turn_context(game, svc_sound, static_cast<std::uint8_t>(VISIT_CONTEXT));
      opeu_location_flash(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(data.event_side));
      port_daily_show(game, VISIT_SHOW, data.event_side, PRESENT_OFFSET, PRESENT_SEGMENT);
      data.side_flags[data.event_side] |= Bits(SideFlag::VISITED);
      opeu_finish_side_initialization(game, svc_sound, static_cast<std::uint8_t>(0));
    }
  }
}

auto port_routine_opeu_visit_event(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_visit_event(game);
  return 0;
}

auto port_routine_opeu_visit_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_visit_present(game);
  return 0;
}
