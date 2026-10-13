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
#include "common/dos-file.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>

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
auto ForEachGeneralOf(Game& game, Unit const& unit, Body&& body) -> void {
  auto&      data{ Data(game) };
  auto const near{ Near(data, unit) };
  auto const selects{ [&game, &data, near](Person const& person) { return person.status != PERSON_VACANT && opeu_person_selected_unit(game, Near(data, person)) == near; } };
  std::ranges::for_each(SideGenerals(data, opeu_unit_side(near)) | std::views::filter(selects), body);
}

auto Lower(Game& game, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  value = SubByte(value, game.State().RandomBounded(spread) + base);
}
auto Raise(Game& game, std::uint8_t& value, std::uint16_t base, std::uint16_t spread) -> void {
  value = AddByte(value, game.State().RandomBounded(spread) + base, RATING_LIMIT);
}

// Whether she visits the unit: one of its generals is in poor health or one of its battalions is a veteran.
auto Eligible(Game& game, Unit const& unit) -> bool {
  bool poor_health{ false };
  ForEachGeneralOf(game, unit, [&poor_health](Person const& person) { poor_health = poor_health || (person.flags & Bits(PersonRole::POOR_HEALTH)) != 0; });
  if (poor_health) return true;
  return std::ranges::any_of(Elements(Data(game), unit), [](Element const& element) { return element.experience >= VETERAN; });
}

// Shows her portrait and one of six remarks; returns the outcome (remark / 2).
auto Remark(Game& game) -> std::uint8_t {
  auto const& data{ Data(game) };
  opeu_message_panel_begin(game);
  opeu_context_frame(game, 44, 282, 18);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ NAME });
  game.Machine().Files().Seek(HandleOf(data.portrait_handle), static_cast<std::int32_t>(std::uint32_t{ PORTRAIT_LOW } | std::uint32_t{ PORTRAIT_HIGH } << 16), services::SeekOrigin::START);
  opeu_image_read_draw(game, 45, 316, 8, 80, 0, data.portrait_handle);
  auto const remark{ static_cast<std::uint8_t>(game.State().RandomBounded(6)) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ BUFFER, REMARK_FORMAT, opeu_message_lookup(game, remark + FIRST_REMARK) });
  opeu_army_position(game, 54, 324);
  opeu_text_feed(game, BUFFER);
  return static_cast<std::uint8_t>(remark >> 1);
}

// Heals the unit's generals (health back to their limit, poor health cleared) and clears its battalions' experience.
void Finish(Game& game, Unit const& unit) {
  ForEachGeneralOf(game, unit, [](Person& person) {
    person.flags  &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
    person.health  = person.max_health;
  });
  std::ranges::for_each(Elements(Data(game), unit), [](Element& element) { element.experience = 0; });
}

// Lowers the general's charm by 1..10 and each battalion's morale by 20..49.
void Demoralize(Game& game, Person& person, Unit const& unit) {
  Lower(game, person.charm, 1, 10);
  std::ranges::for_each(Elements(Data(game), unit), [&game](Element& element) { Lower(game, element.morale, 20, 30); });
  Finish(game, unit);
}

// Raises each battalion's morale by 20..49.
void Inspire(Game& game, Person&, Unit const& unit) {
  std::ranges::for_each(Elements(Data(game), unit), [&game](Element& element) { Raise(game, element.morale, 20, 30); });
  Finish(game, unit);
}

// Demoralizes a unit of side 0; raises the general's charm and the battalions' morale on side 1.
void JudgeSide(Game& game, Person& person, Unit const& unit) {
  if (opeu_unit_side(Near(Data(game), unit)) == 0) {
    Demoralize(game, person, unit);
    return;
  }
  Raise(game, person.charm, 1, 10);
  Inspire(game, person, unit);
}

// The remark's outcomes for the commander and the unit, by the enumerator angel_actions holds for the remark.
constexpr auto ANGEL_ACTIONS = DispatchRows<AngelAction, auto (*)(Game&, Person&, Unit const&)->void>({
    { AngelAction::DEMORALIZE, Demoralize },
    { AngelAction::INSPIRE,    Inspire    },
    { AngelAction::JUDGE_SIDE, JudgeSide  },
});
} // namespace

// The presentation: her portrait and remark, then the outcome the remark chose for the commander's unit.
void opeu_angel_present(Game& game) {
  auto&      data{ Data(game) };
  auto const person{ opeu_find_side_record(game, data.event_side, COMMANDER) };
  auto const unit{ opeu_person_selected_unit(game, person) };
  auto const action{ data.angel_actions.at(Remark(game)) };
  Dispatch(ANGEL_ACTIONS, action, game, Record<Person>(data, person), Record<Unit>(data, unit));
}

void opeu_angel_event(Game& game) {
  auto& data{ Data(game) };
  for (data.event_side = 0; data.event_side < SIDE_COUNT; ++data.event_side) {
    if (data.side_status[data.event_side] != SIDE_PLAYED) continue;
    auto const  unit{ opeu_find_side_selected_unit(game, data.event_side) };
    auto const& record{ Record<Unit>(data, unit) };
    auto const  x{ record.x };
    auto const  y{ record.y };
    if (opeu_map_code(game, x, y) == OPEN_GROUND && game.State().RandomBounded(30) == 3 && Eligible(game, record)) {
      opeu_arrival_location(game, x, y, data.event_side);
      opeu_location_flash(game, x, y, data.event_side);
      opeu_daily_show(game, ANGEL_SHOW, data.event_side, Presenter::ANGEL);
    }
  }
}
