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
#include "common/random.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/msglook.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
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
constexpr std::uint16_t ALL_CATEGORIES{ 255 };
constexpr std::uint8_t  NORTH_ROWS{ 10 };
// The presentations and the far callbacks (segment and offset of opeu_loss_present_a/b) the daily display runs.
constexpr std::uint16_t PRESENT_SEGMENT{ 0x108c };
constexpr std::uint16_t PRESENT_A_ID{ 0x38 };
constexpr std::uint16_t PRESENT_A{ 0x06ac };
constexpr std::uint16_t PRESENT_B_ID{ 0xb2 };
constexpr std::uint16_t PRESENT_B{ 0x0830 };
// The reserves of the locals whose address the services receive: the quantity service's count, the names.
constexpr std::uint16_t WEAK_COUNT{ 0xc };
constexpr std::uint16_t NAME_A{ 0x14 };
constexpr std::uint16_t NAME_B{ 0x10 };

// The format with one word, the word computed before the format is looked up.
auto Format(Game& game, std::uint16_t message, std::uint16_t word) -> void {
  auto const format{ opeu_message_lookup(game, svc_msglook, static_cast<int16_t>(message)) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(word) });
}
} // namespace

// Whether the unit is worn out: by random thresholds on its elements' average morale and experience, or on its
// strength (infantry counting a tenth).
uint16_t opeu_loss_unit_weak(Game& game, uint16_t unit) {
  ServiceLocals const locals{ WEAK_COUNT };
  auto&               data{ Data(game) };
  std::uint16_t       morale{ 0 };
  std::uint16_t       experience{ 0 };
  std::uint16_t       strength{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit).first)) {
    morale     += element.morale;
    experience += element.experience;
    strength   += data.element_types[element.type].category == 0 ? element.quantity / 10 : element.quantity;
  }
  auto const at_count{ locals.At(0) };
  opeu_unit_quantity(game, unit, static_cast<uint8_t>(ALL_CATEGORIES), game.State().Bytes(at_count, 1).data());
  auto const count{ static_cast<std::uint8_t>(Word(game, at_count)) };
  if (!count) return 0;
  if ((RandomBelow(game, 20) + 20 > morale / count && experience / count > 50 - RandomBelow(game, 20)) || RandomBelow(game, 20) + 20 > strength) return 1;
  return 0;
}

void opeu_loss_present_a(Game& game) {
  opeu_sound_cue12(game, svc_soundcue);
  opeu_pending_finish(game, static_cast<uint8_t>(3));
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
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group)
    for (unsigned slot{ 0 }; slot < UNITS_PER_GROUP; ++slot) {
      auto const near{ Near(data, data.units[group * UNITS_PER_GROUP + slot]) };
      if (!(Record<Unit>(data, near).flags & Bits(UnitFlag::READY))) continue;
      if (!opeu_category_7(static_cast<uint8_t>(slot)) && !opeu_loss_unit_weak(game, near)) continue;
      if (RandomBelow(game, 100)) continue;
      if (data.side_status[1] == SIDE_PLAYED && opeu_unit_special(game, near)) {
        auto const unit_name{ opeu_unit_name(game, svc_unitname, near, name) };
        auto const format{ opeu_message_lookup(game, svc_msglook, static_cast<int16_t>(PARTISANS_STRIKE_ENEMY)) };
        opeu_request_format(
            game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(TEXT), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(movement), static_cast<std::uint16_t>(unit_name) });
        opeu_unit_side_report(game, static_cast<const DialogHost*>(port_dialogs_host()), near, TEXT);
        auto const& unit{ Record<Unit>(data, near) };
        opeu_location_flash(game, static_cast<uint8_t>(unit.x), static_cast<uint8_t>(unit.y), static_cast<uint8_t>(1));
        port_daily_show(game, PRESENT_A_ID, 1, PRESENT_A, PRESENT_SEGMENT);
      } else if (data.side_status[0] == SIDE_PLAYED && opeu_unit_selection_eligible(game, near)) {
        Format(game, PARTISANS_STRIKE_US, movement);
        opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), near, TEXT);
        port_daily_show(game, PRESENT_A_ID, 0, PRESENT_A, PRESENT_SEGMENT);
      }
      opeu_music_command7(game, svc_sound);
      opeu_unit_casualties(game, near, 10, 10, 5, 1, 1);
    }
}

// Whether the unit stands north of row 10.
uint16_t opeu_loss_unit_low(Game& game, uint16_t unit) {
  return Record<Unit>(Data(game), unit).y < NORTH_ROWS ? 1 : 0;
}

void opeu_loss_present_b(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(3));
}

void opeu_loss_event_b(Game& game) {
  ServiceLocals const locals{ NAME_B };
  auto&               data{ Data(game) };
  auto const          name{ locals.At(0) };
  if (CurrentScenario(data) != Scenario::D_DAY || (data.weather != 0 && data.weather != 1)) return;
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto const near{ Near(data, data.units[i]) };
    if (!(Record<Unit>(data, near).flags & Bits(UnitFlag::READY)) || !opeu_loss_unit_low(game, near) || RandomBelow(game, 50)) continue;
    if (data.side_status[1] == SIDE_PLAYED && opeu_unit_special(game, near)) {
      Format(game, ENEMY_LOSSES_NORTH, opeu_unit_name(game, svc_unitname, near, name));
      opeu_unit_side_report(game, static_cast<const DialogHost*>(port_dialogs_host()), near, TEXT);
      auto const& unit{ Record<Unit>(data, near) };
      opeu_location_flash(game, static_cast<uint8_t>(unit.x), static_cast<uint8_t>(unit.y), static_cast<uint8_t>(1));
      port_daily_show(game, PRESENT_B_ID, 1, PRESENT_B, PRESENT_SEGMENT);
    } else if (data.side_status[0] == SIDE_PLAYED && opeu_unit_selection_eligible(game, near)) {
      opeu_unit_report(game, static_cast<const DialogHost*>(port_dialogs_host()), near, opeu_message_lookup(game, svc_msglook, static_cast<int16_t>(OUR_LOSSES_NORTH)));
      port_daily_show(game, PRESENT_B_ID, 0, PRESENT_B, PRESENT_SEGMENT);
    }
    opeu_unit_casualties(game, near, 20, 20, 5, 0, 1);
  }
}

auto port_routine_opeu_loss_event_a(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_loss_event_a(game);
  return 0;
}

auto port_routine_opeu_loss_event_b(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_loss_event_b(game);
  return 0;
}

auto port_routine_opeu_loss_present_a(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_loss_present_a(game);
  return 0;
}

auto port_routine_opeu_loss_present_b(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_loss_present_b(game);
  return 0;
}
