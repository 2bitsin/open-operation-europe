// The air strike: a side's strike order (Side::strike: target cell, planes, the hours until it flies and the hours it
// may wait) counts down each hour and asks the air officer whether it flies now, waits or is cancelled. When it flies
// the planes leave the side's bombers, the enemy air defence shoots some down, the map centres on the target and the
// bomber flies down the screen; at the target row it bombs the unit (casualties) and the objective (damage) there,
// or reports that every bomber was lost. The survivors come back to the side at midnight.
//
// A local whose address reaches a service lives in the routine's ServiceLocals reserve.
#include "core/airstrk.hpp"
#include "common/air-flight.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/airshow.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/msnrept.hpp"
#include "core/people.hpp"
#include "core/pixel-service.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/grid.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "map/objdamag.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/value.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t DESTROYED_FORMAT{ offsetof(DataSegment, air_destroyed_format) };
constexpr std::uint16_t STRIKE_NAME{ offsetof(DataSegment, air_strike_name) };
constexpr std::uint16_t STRIKE_BEGIN{ offsetof(DataSegment, air_strike_begin) };
constexpr std::uint16_t STRIKE_SPOTTED{ offsetof(DataSegment, air_strike_spotted) };
constexpr std::uint16_t BOMBERS_NAME{ offsetof(DataSegment, air_bombers_name) };

constexpr std::uint16_t STRONG_AIR_COMMAND{ 0x46 };
constexpr std::uint8_t  HIGH_MORALE{ 0x3c };
constexpr std::uint16_t TEXT_BYTES{ 0x64 };         // opeu_air_destroyed_message's text, which the format service writes
constexpr std::uint16_t BOMB_LEAD{ 0x20 };          // rows above the target where the bombs fall
constexpr std::uint16_t STRIKE_PRESENTER{ 0x0406 }; // opeu_air_strike_present, in the code segment below
constexpr std::uint16_t STRIKE_SEGMENT{ 0x0f43 };

auto IsNight(Game& game) -> bool {
  return opeu_hour_is_night(static_cast<std::uint8_t>(Data(game).hour)) != 0;
}
} // namespace

// How many of the strike's `n` attack waves the enemy shoots down: random rolls, more for a computer side at a high
// delivery level, by day, with a weak air command or low morale, and over an enemy unit or objective.
uint16_t opeu_air_damage_roll(Game& game, uint8_t x, uint8_t y, uint8_t n) {
  auto&         data{ Data(game) };
  auto const    side{ data.air_side };
  auto const    bound{ static_cast<std::uint16_t>((data.side_control[side] == SIDE_PLAYED ? (data.delivery_level >> 1) * n : n) + 1) };
  std::uint16_t lost{ opeu_officer_rating(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(AIR_OFFICER_KIND)) >= STRONG_AIR_COMMAND
                          ? std::uint16_t{ 0 }
                          : static_cast<std::uint16_t>(RandomBelow(game, bound) + 1) };
  if (!IsNight(game)) lost += RandomBelow(game, bound) + 1;
  if (data.sides[side].strength <= HIGH_MORALE) lost += RandomBelow(game, bound) + 1;
  if (auto const foe{ opeu_find_unit_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) }; foe != 0 && static_cast<std::uint8_t>(opeu_unit_side(foe)) != side)
    lost += RandomBelow(game, bound);
  if (auto const objective{ opeu_find_objective_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) };
      objective != 0 && static_cast<std::uint8_t>(opeu_objective_side(game, objective)) != side)
    lost += RandomBelow(game, bound);
  return lost;
}

// "All the <what> were destroyed", from the side's air officer.
void opeu_air_destroyed_message(Game& game, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto const          text{ locals.At(0) };
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(DESTROYED_FORMAT), static_cast<std::uint16_t>(what) });
  auto const officer{ opeu_find_side_record(game, static_cast<std::uint8_t>(Data(game).air_side), static_cast<std::uint8_t>(AIR_OFFICER_KIND)) };
  opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text, officer);
}

// Counts the air side's strike order down an hour; when its delay runs out the air officer decides: 1 when it flies
// now, else it waits another hour or is cancelled.
uint16_t opeu_air_strike_due(Game& game) {
  auto& strike{ AirSide(Data(game)).strike };
  if (!strike.planes) return 0;
  opeu_sub_byte(1, &strike.turns);
  if (!opeu_sub_byte(1, &strike.hours)) switch (static_cast<std::uint8_t>(opeu_air_report(game, STRIKE_NAME, static_cast<std::uint8_t>(strike.turns)))) {
    case 0: return 1;
    case 1: strike.hours = 1; break;
    case 2: strike.planes = 0; break;
    }
  return 0;
}

// The daily-show presenter of a strike: the side's air officer announces it, or the enemy's sees it coming.
void opeu_air_strike_present(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ opeu_find_side_record(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(FLIGHT_COMMANDER_KIND)) };
    auto const officer{ opeu_find_person_template(game, static_cast<std::uint8_t>(Record<Person>(data, commander).status), static_cast<std::uint8_t>(AIR_OFFICER_TEMPLATE)) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), STRIKE_BEGIN, officer);
  } else {
    auto const enemy{ static_cast<std::uint8_t>(opeu_opposite_side(static_cast<std::uint8_t>(side))) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), STRIKE_SPOTTED,
                         opeu_find_side_record(game, static_cast<std::uint8_t>(enemy), static_cast<std::uint8_t>(AIR_OFFICER_KIND)));
  }
  opeu_sound_cue4(game);
}

// The bombs fall on (x, y) from n planes: casualties to the unit there and damage to the objective there, at a rate
// from the air command's supply and the planes, lower at night and under cover (the terrain's for a unit, the
// garrison's for an objective). With neither there the rate is not used.
void opeu_air_strike_hit(Game& game, uint8_t x, uint8_t y, uint16_t n) {
  auto&         data{ Data(game) };
  std::uint16_t shield{ 0 };
  auto const    supply{ opeu_officer_rating(game, static_cast<std::uint8_t>(data.air_side), static_cast<std::uint8_t>(AIR_OFFICER_KIND)) };
  auto const    unit{ opeu_find_unit_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) };
  if (unit) shield = data.air_cover[static_cast<std::uint8_t>(opeu_map_code(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)))] / 6;
  auto const site{ opeu_find_objective_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) };
  if (site) shield = Record<Objective>(data, site).strength / 15;
  auto const root{ opeu_isqrt(n) };
  auto const night{ IsNight(game) };
  auto const rate{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(supply / 10 + n) / static_cast<std::uint16_t>(night + shield + 1))
                                              * static_cast<std::uint32_t>(root)) };
  if (unit) {
    auto const percent{ RandomBelow(game, 60) + 40 };
    opeu_unit_casualties(game, unit, percent, rate, n, 0, 1);
  }
  if (site) {
    auto const divisor{ static_cast<std::uint16_t>(RandomBelow(game, 2) + 1) };
    auto const works_night{ IsNight(game) };
    auto const works_root{ opeu_isqrt(n) };
    auto const works{ static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(works_root) * supply) * static_cast<std::uint32_t>(n)) / (works_night + 1))
        / divisor) };
    auto const stores{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(n / 5) * n) / (IsNight(game) + 1)) };
    opeu_objective_damage(game, site, 0x5a, rate, works, stores, n, 0);
  }
}

// The strike flies: the planes leave the side's bombers, the air defence takes its toll, and the bomber crosses the
// screen over the target; the survivors return at midnight and the order is cleared.
void opeu_air_strike_event(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  opeu_set_turn_context(game, static_cast<std::uint8_t>(data.side_status[side] == SIDE_PLAYED ? 0xd : 0xe));
  auto&         record{ data.sides[side] };
  auto const    x{ record.strike.x };
  auto const    y{ record.strike.y };
  std::uint16_t planes{ record.strike.planes };
  opeu_sub_word(planes, &record.bombers);
  opeu_sub_word(opeu_air_damage_roll(game, x, y, static_cast<std::uint8_t>(opeu_min16(planes, 20) / 7)), &planes);
  auto const view{ data.map_mode[0] };
  data.map_mode[0] = 1;
  opeu_map_center(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(static_cast<std::uint8_t>(opeu_side_for_status(game, static_cast<std::uint8_t>(side)))));
  auto const pixel{ CellViewPixel(game, x, y) };
  auto const hx{ pixel.x };
  auto const my{ pixel.y };
  for (std::uint16_t row{ FLIGHT_TOP_ROW }; row >= FLIGHT_BOTTOM_ROW; --row) {
    opeu_air_sprite_draw(game, 0, hx, row, static_cast<std::uint8_t>(side));
    if (row == my) {
      auto const shown{ static_cast<std::uint8_t>(opeu_side_for_status(game, static_cast<std::uint8_t>(side))) };
      opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), side == 0 ? 0x1e : 0xf, static_cast<std::uint8_t>(shown),
                      static_cast<std::uint32_t>(STRIKE_SEGMENT) << 16 | STRIKE_PRESENTER);
    }
    if (my - row == BOMB_LEAD) {
      opeu_music_command7(game);
      if (!planes) {
        opeu_air_destroyed_message(game, BOMBERS_NAME);
        opeu_air_sprite_restore(game, 0, hx, row);
        break;
      }
      opeu_air_strike_hit(game, x, y, planes);
    }
    opeu_delay_ticks(game, 2);
    opeu_air_sprite_restore(game, 0, hx, row);
  }
  data.map_mode[0] = view;
  if (planes) {
    opeu_officer_gain(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(AIR_OFFICER_KIND));
    record.pending_bombers = static_cast<std::uint16_t>(record.pending_bombers + planes);
  }
  record.strike.planes = 0;
  opeu_finish_side_initialization(game, static_cast<std::uint8_t>(0));
  opeu_music_command7(game);
  opeu_music_stop(game);
  opeu_turn_pre_context(game);
}

auto port_routine_opeu_air_strike_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_air_strike_present(game);
  return 0;
}
