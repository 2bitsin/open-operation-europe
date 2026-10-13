// The commander's visit: in clear weather a side the player watches may be visited (1 in 70 each hour) when its
// general is fit (health 30 or more, not in poor health) and his unit is supported, safe from enemy special units
// and on open ground of the side's influence. A visitor chats with him and his ratings rise.
#include "core/evvisit.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "core/grid-neighbours.hpp"
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

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>

using namespace opeu::game;

namespace {

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t VISIT_FORMAT{ offsetof(DataSegment, visit_format) }; // "At ease, Commander %s."
constexpr std::uint16_t VISIT_SHOW{ 0x88 };
constexpr std::uint16_t VISIT_CONTEXT{ 15 };
constexpr std::uint8_t  COMMANDER{ 1 };   // the side record kind of the commander
constexpr std::uint8_t  VISITOR{ 4 };     // the person template kind of the visitor
constexpr std::uint8_t  OPEN_GROUND{ 1 }; // the map code of a cell the visit happens on
constexpr std::uint8_t  MIN_HEALTH{ 30 };

auto Raise(Game& game, std::uint8_t& rating) -> void {
  opeu_add_byte(RandomBelow(game, 10), RATING_LIMIT, &rating);
}

// Whether an enemy special unit stands next to the unit.
auto Threatened(Game& game, Unit const& unit, std::uint8_t enemy) -> bool {
  auto const special_enemy{ [&game, enemy](GridCell cell) { return opeu_cell_side(game, cell.x, cell.y) == enemy && opeu_unit_special(game, opeu_find_unit_at(game, cell.x, cell.y)); } };
  return std::ranges::any_of(CellsAround({ unit.x, unit.y }), special_enemy);
}

// Raises leadership, charm, bravery, infantry, artillery and armor by 0..9 each, up to 99.
void Boost(Game& game, Person& general) {
  Raise(game, general.leadership);
  Raise(game, general.charm);
  Raise(game, general.bravery);
  Raise(game, general.infantry);
  Raise(game, general.artillery);
  Raise(game, general.armor);
}

// One of three conversations with the visitor of the commander's nation, then the commander's ratings rise.
void Converse(Game& game, uint8_t side) {
  auto const commander{ opeu_find_side_record(game, side, COMMANDER) };
  auto const speaker{ opeu_find_person_template(game, Record<Person>(Data(game), opeu_find_side_record(game, side, COMMANDER)).status, VISITOR) };
  auto const say{ [&](std::uint16_t text) { opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, speaker); } };
  auto const lookup{ [&](std::uint16_t id) -> std::uint16_t { return opeu_message_lookup(game, id); } };
  switch (RandomBelow(game, 3)) {
  case 0:
    opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, VISIT_FORMAT, commander });
    say(TEXT);
    std::ranges::for_each(std::views::iota(std::uint16_t{ 12 }, std::uint16_t{ 17 }), [&](std::uint16_t id) { say(lookup(id)); });
    break;
  case 1:
    say(lookup(17));
    opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, lookup(18), commander });
    say(TEXT);
    say(lookup(19));
    say(lookup(20));
    say(lookup(22));
    break;
  case 2:
    opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, lookup(23), commander });
    say(TEXT);
    std::ranges::for_each(std::views::iota(std::uint16_t{ 24 }, std::uint16_t{ 27 }), [&](std::uint16_t id) { say(lookup(id)); });
    break;
  }
  Boost(game, Record<Person>(Data(game), commander));
}
} // namespace

// Whether no enemy special unit stands next to the unit and its cell is open ground outside the enemy's influence.
auto opeu_visit_unit_safe(Game& game, Unit const& unit) -> bool {
  auto const enemy{ opeu_opposite_side(opeu_unit_side(Near(Data(game), unit))) };
  return !Threatened(game, unit, enemy) && opeu_map_code(game, unit.x, unit.y) == OPEN_GROUND && opeu_cell_influence_side(game, unit.x, unit.y) != enemy;
}

// The presentation (Presenter::VISIT): the conversation, then the commander panel shown afresh.
void opeu_visit_present(Game& game) {
  auto& data{ Data(game) };
  Converse(game, data.event_side);
  data.context_side = NO_SIDE;
  opeu_daily_context(game, data.event_side);
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
    if (!data.weather && opeu_unit_support_condition(game, unit) && opeu_visit_unit_safe(game, Record<Unit>(data, unit)) && RandomBelow(game, 70) == 1) {
      auto const& target{ Record<Unit>(data, unit) };
      auto const  x{ target.x };
      auto const  y{ target.y };
      opeu_set_turn_context(game, VISIT_CONTEXT);
      opeu_location_flash(game, x, y, data.event_side);
      port_daily_show(game, VISIT_SHOW, data.event_side, Presenter::VISIT);
      data.side_flags[data.event_side] |= Bits(SideFlag::VISITED);
      opeu_finish_side_initialization(game, 0);
    }
  }
}
