// Hourly unit losses. In scenarios 2-4 the partisans or the resistance strike, one time in a hundred, any ready
// regiment in a category-7 slot or one judged worn out; in scenario 3 in clear or cloudy weather, regiments north of
// row 10 are hit one time in fifty. The side concerned is told through the daily display and the regiment takes
// casualties.
#include "core/evloss.hpp"
#include "common/events.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/mapunit.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>

using namespace opeu::game;

namespace {

// The messages of the events.
enum Message : std::uint16_t {
  PARTISANS_STRIKE_ENEMY = 29, // formatted with the movement and the enemy regiment's name
  PARTISANS_STRIKE_US    = 30, // formatted with the movement
  ENEMY_LOSSES_NORTH     = 31, // formatted with the enemy regiment's name
  OUR_LOSSES_NORTH       = 32,
};

constexpr std::uint16_t TEXT{ offsetof(DataSegment, event_buffer) };
constexpr std::uint8_t  NORTH_ROWS{ 10 };
// The daily shows of the two presentations.
constexpr std::uint16_t PRESENT_A_ID{ 0x38 };
constexpr std::uint16_t PRESENT_B_ID{ 0xb2 };
// The reserves of the locals whose address the name services receive.
constexpr std::uint16_t NAME_A{ 0x14 };
constexpr std::uint16_t NAME_B{ 0x10 };

// The format with one word, the word computed before the format is looked up.
auto Format(Game& game, std::uint16_t message, std::uint16_t word) -> void {
  auto const format{ opeu_message_lookup(game, message) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ TEXT, format, word });
}

// Whether the unit is worn out: by random thresholds on its elements' average morale and experience, or on its
// strength (infantry counting a tenth).
auto Weak(Game& game, Unit const& unit) -> bool {
  auto&         data{ Data(game) };
  std::uint16_t morale{ 0 };
  std::uint16_t experience{ 0 };
  std::uint16_t strength{ 0 };
  for (auto const& element : Elements(data, unit)) {
    morale     += element.morale;
    experience += element.experience;
    strength   += data.element_types[element.type].category == 0 ? element.quantity / 10 : element.quantity;
  }
  auto const count{ UnitQuantity(data, Near(data, unit), ANY_CATEGORY).count };
  if (!count) return false;
  return (game.State().RandomBounded(20) + 20 > morale / count && experience / count > 50 - game.State().RandomBounded(20)) || game.State().RandomBounded(20) + 20 > strength;
}

// Whether the unit stands north of row 10.
auto North(Unit const& unit) -> bool {
  return unit.y < NORTH_ROWS;
}

// The first side's regiments with their slot in their division.
auto FirstSideUnits(DataSegment& data) {
  return std::views::zip(std::views::iota(0U), std::span{ data.units }.first(GROUPS_PER_SIDE * UNITS_PER_GROUP))
         | std::views::transform([](auto const& indexed) { return std::pair<std::uint8_t, Unit&>{ static_cast<std::uint8_t>(std::get<0>(indexed) % UNITS_PER_GROUP), std::get<1>(indexed) }; });
}

// The partisans or the resistance (movement) strike the regiment.
void Strike(Game& game, Unit const& unit, std::uint16_t movement, std::uint16_t name) {
  auto&      data{ Data(game) };
  auto const near{ Near(data, unit) };
  if (data.side_status[1] == SIDE_PLAYED && opeu_unit_special(game, near)) {
    auto const unit_name{ opeu_unit_name(game, near, name) };
    auto const format{ opeu_message_lookup(game, static_cast<int16_t>(PARTISANS_STRIKE_ENEMY)) };
    opeu_request_format(game, std::array<std::uint16_t, 4>{ TEXT, format, movement, unit_name });
    opeu_unit_side_report(game, near, TEXT);
    opeu_location_flash(game, unit.x, unit.y, 1);
    opeu_daily_show(game, PRESENT_A_ID, 1, Presenter::PARTISAN_LOSSES);
  } else if (data.side_status[0] == SIDE_PLAYED && opeu_unit_selection_eligible(game, near)) {
    Format(game, PARTISANS_STRIKE_US, movement);
    opeu_unit_report(game, near, TEXT);
    opeu_daily_show(game, PRESENT_A_ID, 0, Presenter::PARTISAN_LOSSES);
  }
  opeu_music_command7(game);
  opeu_unit_casualties(game, near, 10, 10, 5, 1, 1);
}

// The regiment north of row 10 takes losses.
void NorthernLosses(Game& game, Unit const& unit, std::uint16_t name) {
  auto&      data{ Data(game) };
  auto const near{ Near(data, unit) };
  if (data.side_status[1] == SIDE_PLAYED && opeu_unit_special(game, near)) {
    Format(game, ENEMY_LOSSES_NORTH, opeu_unit_name(game, near, name));
    opeu_unit_side_report(game, near, TEXT);
    opeu_location_flash(game, unit.x, unit.y, 1);
    opeu_daily_show(game, PRESENT_B_ID, 1, Presenter::NORTHERN_LOSSES);
  } else if (data.side_status[0] == SIDE_PLAYED && opeu_unit_selection_eligible(game, near)) {
    opeu_unit_report(game, near, opeu_message_lookup(game, static_cast<int16_t>(OUR_LOSSES_NORTH)));
    opeu_daily_show(game, PRESENT_B_ID, 0, Presenter::NORTHERN_LOSSES);
  }
  opeu_unit_casualties(game, near, 20, 20, 5, 0, 1);
}
} // namespace

void opeu_loss_present_a(Game& game) {
  opeu_sound_cue12(game);
  opeu_pending_finish(game, 3);
}

void opeu_loss_event_a(Game& game) {
  ServiceLocals const locals{ NAME_A };
  auto&               data{ Data(game) };
  auto const          name{ locals.At(0) };
  auto const          scenario{ CurrentScenario(data) };
  std::uint16_t       movement{};
  if (scenario == Scenario::KURSK)
    movement = data.resistance_names[0];
  else if (scenario == Scenario::D_DAY || scenario == Scenario::BULGE)
    movement = data.resistance_names[1];
  else
    return;
  for (auto const [slot, unit] : FirstSideUnits(data)) {
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    if (!opeu_category_7(slot) && !Weak(game, unit)) continue;
    if (game.State().RandomBounded(100)) continue;
    Strike(game, unit, movement, name);
  }
}

void opeu_loss_present_b(Game& game) {
  opeu_pending_finish(game, 3);
}

void opeu_loss_event_b(Game& game) {
  ServiceLocals const locals{ NAME_B };
  auto&               data{ Data(game) };
  auto const          name{ locals.At(0) };
  if (CurrentScenario(data) != Scenario::D_DAY || (data.weather != 0 && data.weather != 1)) return;
  for (auto const& unit : std::span{ data.units }.first(UNITS_PER_SIDE))
    if ((unit.flags & Bits(UnitFlag::READY)) && North(unit) && !game.State().RandomBounded(50)) NorthernLosses(game, unit, name);
}
