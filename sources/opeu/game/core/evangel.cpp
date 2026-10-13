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
#include "common/dispatch.hpp"
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

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t BUFFER{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t NAME{ offsetof(DataSegment, angel_name) };
constexpr std::uint16_t REMARK_FORMAT{ offsetof(DataSegment, angel_format) };
constexpr std::uint16_t PORTRAIT_LOW{ 0x7c80 }; // her portrait's position in the portrait file
constexpr std::uint16_t PORTRAIT_HIGH{ 3 };
constexpr std::uint16_t FIRST_REMARK{ 37 };
constexpr std::uint16_t ANGEL_SHOW{ 215 };
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

uint16_t opeu_angel_unit_eligible(Game& game, uint16_t unit) {
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
uint8_t opeu_angel_portrait(Game& game, uint8_t) {
  auto const& data{ Data(game) };
  opeu_message_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
  opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 44, 282, 18);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(NAME) });
  opeu_dos_seek(static_cast<int16_t>(data.portrait_handle),
                static_cast<int32_t>(static_cast<std::uint32_t>(static_cast<std::uint16_t>(PORTRAIT_LOW)) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(PORTRAIT_HIGH)) << 16),
                static_cast<int16_t>(0));
  opeu_image_read_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 45, 316, 8, 80, 0, static_cast<int16_t>(data.portrait_handle));
  auto const remark{ static_cast<std::uint8_t>(RandomBelow(game, 6)) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(REMARK_FORMAT),
                                                          static_cast<std::uint16_t>(opeu_message_lookup(game, static_cast<int16_t>(remark + FIRST_REMARK))) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(54), static_cast<int16_t>(324));
  opeu_text_feed(game, static_cast<const DialogHost*>(port_dialogs_host()), BUFFER);
  return static_cast<std::uint8_t>(remark >> 1);
}

// Heals the unit's generals (health back to their limit, poor health cleared) and clears its battalions' experience.
void opeu_angel_finish(Game& game, uint16_t, uint16_t unit) {
  ForEachGeneralOf(game, unit, [](Person& person) {
    person.flags  &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
    person.health  = person.max_health;
  });
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) element.experience = 0;
}

// Lowers the general's charm by 1..10 and each battalion's morale by 20..49.
void opeu_angel_demoralize(Game& game, uint16_t person, uint16_t unit) {
  auto& data{ Data(game) };
  Lower(game, Record<Person>(data, person).charm, 1, 10);
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Lower(game, element.morale, 20, 30);
  opeu_angel_finish(game, person, unit);
}

// Raises each battalion's morale by 20..49.
void opeu_angel_inspire(Game& game, uint16_t person, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) Raise(game, element.morale, 20, 30);
  opeu_angel_finish(game, person, unit);
}

// Demoralizes a unit of side 0; raises the general's charm and the battalions' morale on side 1.
void opeu_angel_judge_side(Game& game, uint16_t person, uint16_t unit) {
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
  opeu_angel_finish(game, person, unit);
}

namespace {
// The remark's outcomes for the commander and the unit, by the enumerator angel_actions holds for the remark.
constexpr auto ANGEL_ACTIONS = DispatchRows<AngelAction, auto (*)(Game&, uint16_t, uint16_t)->void>({
    { AngelAction::DEMORALIZE, opeu_angel_demoralize },
    { AngelAction::INSPIRE,    opeu_angel_inspire    },
    { AngelAction::JUDGE_SIDE, opeu_angel_judge_side },
});
} // namespace

// The presentation: her portrait and remark, then the outcome the remark chose for the commander's unit.
void opeu_angel_present(Game& game) {
  auto&      data{ Data(game) };
  auto const person{ opeu_find_side_record(game, data.event_side, COMMANDER) };
  auto const unit{ opeu_person_selected_unit(game, person) };
  auto const action{ data.angel_actions.at(opeu_angel_portrait(game, data.event_side)) };
  Dispatch(ANGEL_ACTIONS, action, game, person, unit);
}

void opeu_angel_event(Game& game) {
  auto& data{ Data(game) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SIDE_PLAYED) continue;
    auto const  unit{ opeu_find_side_selected_unit(game, data.event_side) };
    auto const& record{ Record<Unit>(data, unit) };
    auto const  x{ record.x };
    auto const  y{ record.y };
    if (opeu_map_code(game, x, y) == OPEN_GROUND && RandomBelow(game, 30) == 3 && opeu_angel_unit_eligible(game, unit)) {
      opeu_arrival_location(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(data.event_side));
      opeu_location_flash(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(data.event_side));
      port_daily_show(game, ANGEL_SHOW, data.event_side, Presenter::ANGEL);
    }
  }
}
