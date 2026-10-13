// The paratroop drop: counting a pending drop down to its hour, flying the transports over the target, landing the
// paratroop regiment, and calling the drop off.
#include "core/paradrop.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "types.hpp"

#include <cstddef>
#include <utility>

#include "common/game.hpp"
#include "core/airshow.hpp"
#include "core/airstrk.hpp"
#include "core/dialogs.hpp"
#include "core/msnrept.hpp"
#include "core/paraunit.hpp"
#include "core/people.hpp"
#include "core/pixel-service.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/maperase.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// AIR_REPORT's answers.
enum class AirAnswer : uint8_t {
  FLY      = 0,
  WAIT     = 1,
  CALL_OFF = 2,
};

// The drop is announced by the army commander's nation's air force commander, the enemy planes are reported by the
// other side's intelligence chief, and the intelligence chief gains experience from a drop that brings planes home.
constexpr auto COMMANDER{ std::to_underlying(StaffRole::ARMY_COMMANDER) };
constexpr auto AIR_COMMANDER{ std::to_underlying(StaffRole::AIR_FORCE_COMMANDER) };
constexpr auto INTELLIGENCE{ std::to_underlying(StaffRole::INTELLIGENCE_CHIEF) };

// The transports' flight: from pixel row 0x15c up to 0x54; with every plane lost below row 0x114 the flight ends.
constexpr uint16_t FLIGHT_START{ 0x15c };
constexpr uint16_t FLIGHT_END{ 0x54 };
constexpr uint16_t ALL_LOST_ROW{ 0x114 };
// The regiment lands this many rows after the planes pass over the target.
constexpr int LANDING_ROWS{ 0x20 };
// Anti-aircraft fire counts at most this many planes.
constexpr uint16_t FIRE_PLANES{ 0x59 };
constexpr uint16_t SHOW_ID{ 0xfa };

// Lands the side's paratroop regiment of the drop (a DS offset) at the drop's cell; returns the regiment.
auto CreateDropUnit(Game& game, uint8_t side, uint16_t drop) -> uint16_t {
  auto const& planned{ Record<AirDrop>(Data(game), drop) };
  return opeu_para_unit_create(game, side, planned.x, planned.y);
}

} // namespace

// Calls off the air side's drop: no transports, and the regiments and battalions set aside are released.
void opeu_para_clear(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  data.sides[side].drop.transports = 0;
  for (uint8_t i = 0; i < UNITS_PER_SIDE; i++) {
    auto& unit{ data.units[side * UNITS_PER_SIDE + i] };
    if (unit.flags & Bits(UnitFlag::AIRBORNE)) unit.flags = 0;
  }
  for (uint8_t i = 0; i < ELEMENTS_PER_SIDE; i++) {
    auto& element{ data.elements[side * ELEMENTS_PER_SIDE + i] };
    if (element.flags & Bits(ElementFlag::AIRBORNE)) element.flags = 0;
  }
}

// Whether the paratroop regiment's division is active: nonzero when it is, and also when no regiment is set aside.
uint16_t opeu_para_ready(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  for (uint8_t i = 0; i < UNITS_PER_SIDE; i++) {
    auto const& unit{ data.units[side * UNITS_PER_SIDE + i] };
    if (unit.flags & Bits(UnitFlag::AIRBORNE)) {
      auto const group{ opeu_unit_group(Near(data, unit)) };
      return (Record<Group>(data, group).flags & Bits(GroupFlag::READY)) ? 1 : 0;
    }
  }
  return 1;
}

// Counts the air side's drop down an hour. A drop whose regiment is not ready or whose target cell no longer takes
// paratroops is called off; when its hour comes the air command decides: fly now (returns 1), hold it an hour, or
// call it off.
uint16_t opeu_para_drop_due(Game& game) {
  auto& data{ Data(game) };
  auto& drop{ data.sides[data.air_side].drop };
  if (!drop.transports) return 0;
  opeu_sub_byte(1, &drop.due);
  if (!opeu_para_ready(game) || !opeu_para_cell_ok(game, static_cast<uint8_t>(drop.x), static_cast<uint8_t>(drop.y)))
    opeu_para_clear(game);
  else if (!opeu_sub_byte(1, &drop.hours))
    switch (static_cast<AirAnswer>(static_cast<std::uint16_t>(opeu_air_report(game, Near(data, data.paratroop_drop_name), static_cast<uint8_t>(drop.due))) & 0xff)) {
    case AirAnswer::FLY:      return 1;
    case AirAnswer::WAIT:     drop.hours = 1; break;
    case AirAnswer::CALL_OFF: opeu_para_clear(game); break;
    }
  return 0;
}

// The report shown when the transports reach the target: the side's air commander announces the drop to a player's
// side, the other side's intelligence chief reports the enemy planes.
void opeu_para_drop_present(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ opeu_find_side_record(game, static_cast<uint8_t>(side), static_cast<uint8_t>(COMMANDER)) };
    auto const person{ opeu_find_person_template(game, static_cast<uint8_t>(Record<Person>(data, commander).status), static_cast<uint8_t>(AIR_COMMANDER)) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.paratroop_drop_begin), person);
  } else {
    auto const other{ static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(side))) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), Near(data, data.paratroop_drop_spotted),
                         opeu_find_side_record(game, static_cast<uint8_t>(other), static_cast<uint8_t>(INTELLIGENCE)));
  }
  opeu_sound_cue4(game);
}

// Flies the air side's drop: centres the map on the target, books the transports and the paratroops out of the
// side's totals, takes the anti-aircraft losses, and moves the formation up the screen. Over the target the drop's
// report is shown; a little later the paratroop regiment is formed and lands with the losses of the planes shot
// down. The surviving transports come back to the side, and the drop is cleared.
void opeu_para_drop_event(Game& game) {
  auto&      data{ Data(game) };
  auto const side_index{ data.air_side };
  auto&      side{ data.sides[side_index] };
  auto&      drop{ side.drop };
  auto const x{ drop.x };
  auto const y{ drop.y };
  auto const was{ data.map_mode[0] };
  data.map_mode[0] = 1;
  opeu_map_center(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(static_cast<std::uint16_t>(opeu_side_for_status(game, static_cast<uint8_t>(side_index)))));
  uint16_t left{ drop.transports };
  opeu_sub_word(left, &side.planes);
  opeu_sub_word(drop.paratroops, &side.paratroops);
  opeu_sub_word(opeu_air_damage_roll(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(opeu_min16(left, FIRE_PLANES) / 30)), &left);
  auto const     pixel{ CellViewPixel(game, x, y) };
  uint16_t const target_x{ pixel.x };
  uint16_t const target_y{ pixel.y };
  for (uint16_t row = FLIGHT_START; row >= FLIGHT_END; row--) {
    if (row <= ALL_LOST_ROW && left == 0) {
      opeu_air_destroyed_message(game, Near(data, data.paratroop_planes_name));
      break;
    }
    opeu_air_sprite_draw(game, 0, target_x, row, static_cast<uint8_t>(side_index));
    if (row == target_y) port_daily_show(game, SHOW_ID, static_cast<std::uint16_t>(opeu_side_for_status(game, static_cast<uint8_t>(side_index))), Presenter::PARATROOPERS);
    if (target_y - row == LANDING_ROWS) {
      opeu_music_command7(game);
      auto const unit{ CreateDropUnit(game, static_cast<uint8_t>(side_index), Near(data, drop)) };
      if (opeu_para_land(game, unit, (drop.transports - left) * 10)) opeu_arrival_prepare(game, unit);
    }
    opeu_delay_ticks(game, 1);
    opeu_air_sprite_restore(game, 0, target_x, row);
  }
  data.map_mode[0] = was;
  if (left) {
    opeu_officer_gain(game, static_cast<uint8_t>(side_index), static_cast<uint8_t>(INTELLIGENCE));
    side.pending_planes += left;
  }
  opeu_para_clear(game);
  drop.transports = 0;
}
