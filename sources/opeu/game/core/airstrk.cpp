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
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/grid.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "map/objdamag.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/casualty.hpp"
#include "units/value.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the strike calls through its event.
enum Service : std::uint16_t {
  OFFICER_RATING   = 0,  // (side, kind)
  NIGHT            = 1,  // (hour) -> nonzero outside 5..19
  UNIT_AT          = 2,  // (x, y) -> unit or 0
  UNIT_SIDE        = 3,  // (unit)
  OBJECTIVE_AT     = 4,  // (x, y) -> objective or 0
  OBJECTIVE_SIDE   = 5,  // (objective)
  REQUEST_FORMAT   = 6,  // (destination, format, words...)
  SIDE_RECORD      = 7,  // (side, rank) -> person
  MESSAGE          = 8,  // (text, person)
  AIR_REPORT       = 9,  // (hours left, mission name) -> 0 fly now, 1 wait an hour, 2 cancel
  PERSON_TEMPLATE  = 10, // (nation, kind) -> person template
  OPPOSITE_SIDE    = 11, // (side)
  SOUND_CUE        = 12, // ()
  MAP_CODE         = 13, // (x, y) -> terrain code
  ISQRT            = 14, // (value)
  CASUALTIES       = 15, // (unit, percent, rate, planes, 0, 1)
  OBJECTIVE_DAMAGE = 16, // (objective, percent, rate, works, stores, planes, 0)
  TURN_CONTEXT     = 17, // (context)
  SIDE_FOR_STATUS  = 18, // (side)
  MAP_CENTER       = 19, // (x, y, side)
  VIEW_PIXEL       = 20, // (&x, &y): a cell to its screen pixel
  SPRITE_DRAW      = 21, // (x, y, side)
  DAILY_SHOW       = 22, // (picture, side, presenter offset, presenter segment)
  MUSIC_COMMAND7   = 23, // ()
  DELAY            = 24, // (ticks)
  SPRITE_RESTORE   = 25, // (x, y)
  OFFICER_GAIN     = 26, // (side, kind)
  FINISH_SIDE      = 27, // (value)
  MUSIC_STOP       = 28, // ()
  TURN_PRE_CONTEXT = 29, // ()
};

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

auto AirSide(DataSegment& data) -> Side& {
  return data.sides[data.air_side];
}

auto IsNight(std::uint8_t* ds, HostEvent event) -> bool {
  return Call(ds, event, NIGHT, Data(ds).hour) != 0;
}
} // namespace

// How many of the strike's `n` attack waves the enemy shoots down: random rolls, more for a computer side at a high
// delivery level, by day, with a weak air command or low morale, and over an enemy unit or objective.
uint16_t opeu_air_damage_roll(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y, uint8_t n) {
  auto&         data{ Data(ds) };
  auto const    side{ data.air_side };
  auto const    bound{ static_cast<std::uint16_t>((data.side_control[side] == SIDE_PLAYED ? (data.delivery_level >> 1) * n : n) + 1) };
  std::uint16_t lost{ Call(ds, event, OFFICER_RATING, side, AIR_OFFICER_KIND) >= STRONG_AIR_COMMAND ? std::uint16_t{ 0 } : static_cast<std::uint16_t>(RandomBelow(ds, bound) + 1) };
  if (!IsNight(ds, event)) lost += RandomBelow(ds, bound) + 1;
  if (data.sides[side].strength <= HIGH_MORALE) lost += RandomBelow(ds, bound) + 1;
  if (auto const foe{ Call(ds, event, UNIT_AT, x, y) }; foe != 0 && static_cast<std::uint8_t>(Call(ds, event, UNIT_SIDE, foe)) != side) lost += RandomBelow(ds, bound);
  if (auto const objective{ Call(ds, event, OBJECTIVE_AT, x, y) }; objective != 0 && static_cast<std::uint8_t>(Call(ds, event, OBJECTIVE_SIDE, objective)) != side) lost += RandomBelow(ds, bound);
  return lost;
}

// "All the <what> were destroyed", from the side's air officer.
void opeu_air_destroyed_message(uint8_t ds[65536], HostEvent event, uint16_t what) {
  ServiceLocals const locals{ TEXT_BYTES };
  auto const          text{ locals.At(0) };
  Call(ds, event, REQUEST_FORMAT, text, DESTROYED_FORMAT, what);
  auto const officer{ Call(ds, event, SIDE_RECORD, Data(ds).air_side, AIR_OFFICER_KIND) };
  Call(ds, event, MESSAGE, text, officer);
}

// Counts the air side's strike order down an hour; when its delay runs out the air officer decides: 1 when it flies
// now, else it waits another hour or is cancelled.
uint16_t opeu_air_strike_due(uint8_t ds[65536], HostEvent event) {
  auto& strike{ AirSide(Data(ds)).strike };
  if (!strike.planes) return 0;
  opeu_sub_byte(1, &strike.turns);
  if (!opeu_sub_byte(1, &strike.hours)) switch (static_cast<std::uint8_t>(Call(ds, event, AIR_REPORT, strike.turns, STRIKE_NAME))) {
    case 0: return 1;
    case 1: strike.hours = 1; break;
    case 2: strike.planes = 0; break;
    }
  return 0;
}

// The daily-show presenter of a strike: the side's air officer announces it, or the enemy's sees it coming.
void opeu_air_strike_present(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ Call(ds, event, SIDE_RECORD, side, FLIGHT_COMMANDER_KIND) };
    auto const officer{ Call(ds, event, PERSON_TEMPLATE, Record<Person>(data, commander).status, AIR_OFFICER_TEMPLATE) };
    Call(ds, event, MESSAGE, STRIKE_BEGIN, officer);
  } else {
    auto const enemy{ static_cast<std::uint8_t>(Call(ds, event, OPPOSITE_SIDE, side)) };
    Call(ds, event, MESSAGE, STRIKE_SPOTTED, Call(ds, event, SIDE_RECORD, enemy, AIR_OFFICER_KIND));
  }
  Call(ds, event, SOUND_CUE);
}

// The bombs fall on (x, y) from n planes: casualties to the unit there and damage to the objective there, at a rate
// from the air command's supply and the planes, lower at night and under cover (the terrain's for a unit, the
// garrison's for an objective). With neither there the rate is not used.
void opeu_air_strike_hit(uint8_t ds[65536], HostEvent event, uint8_t x, uint8_t y, uint16_t n) {
  auto&         data{ Data(ds) };
  std::uint16_t shield{ 0 };
  auto const    supply{ Call(ds, event, OFFICER_RATING, data.air_side, AIR_OFFICER_KIND) };
  auto const    unit{ Call(ds, event, UNIT_AT, x, y) };
  if (unit) shield = data.air_cover[static_cast<std::uint8_t>(Call(ds, event, MAP_CODE, x, y))] / 6;
  auto const site{ Call(ds, event, OBJECTIVE_AT, x, y) };
  if (site) shield = Record<Objective>(data, site).strength / 15;
  auto const root{ Call(ds, event, ISQRT, n) };
  auto const night{ IsNight(ds, event) };
  auto const rate{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(supply / 10 + n) / static_cast<std::uint16_t>(night + shield + 1))
                                              * static_cast<std::uint32_t>(root)) };
  if (unit) {
    auto const percent{ RandomBelow(ds, 60) + 40 };
    Call(ds, event, CASUALTIES, unit, percent, rate, n, 0, 1);
  }
  if (site) {
    auto const divisor{ static_cast<std::uint16_t>(RandomBelow(ds, 2) + 1) };
    auto const works_night{ IsNight(ds, event) };
    auto const works_root{ Call(ds, event, ISQRT, n) };
    auto const works{ static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(works_root) * supply) * static_cast<std::uint32_t>(n)) / (works_night + 1))
        / divisor) };
    auto const stores{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(n / 5) * n) / (IsNight(ds, event) + 1)) };
    Call(ds, event, OBJECTIVE_DAMAGE, site, 0x5a, rate, works, stores, n, 0);
  }
}

// The strike flies: the planes leave the side's bombers, the air defence takes its toll, and the bomber crosses the
// screen over the target; the survivors return at midnight and the order is cleared.
void opeu_air_strike_event(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  Call(ds, event, TURN_CONTEXT, data.side_status[side] == SIDE_PLAYED ? 0xd : 0xe);
  auto&         record{ data.sides[side] };
  auto const    x{ record.strike.x };
  auto const    y{ record.strike.y };
  std::uint16_t planes{ record.strike.planes };
  opeu_sub_word(planes, &record.bombers);
  opeu_sub_word(opeu_air_damage_roll(ds, event, x, y, static_cast<std::uint8_t>(opeu_min16(planes, 20) / 7)), &planes);
  auto const view{ data.map_mode[0] };
  data.map_mode[0] = 1;
  Call(ds, event, MAP_CENTER, x, y, static_cast<std::uint8_t>(Call(ds, event, SIDE_FOR_STATUS, side)));
  auto const pixel{ ServicePixel(ds, event, VIEW_PIXEL, x, y) };
  auto const hx{ pixel.x };
  auto const my{ pixel.y };
  for (std::uint16_t row{ FLIGHT_TOP_ROW }; row >= FLIGHT_BOTTOM_ROW; --row) {
    Call(ds, event, SPRITE_DRAW, hx, row, side);
    if (row == my) {
      auto const shown{ static_cast<std::uint8_t>(Call(ds, event, SIDE_FOR_STATUS, side)) };
      Call(ds, event, DAILY_SHOW, side == 0 ? 0x1e : 0xf, shown, STRIKE_PRESENTER, STRIKE_SEGMENT);
    }
    if (my - row == BOMB_LEAD) {
      Call(ds, event, MUSIC_COMMAND7);
      if (!planes) {
        opeu_air_destroyed_message(ds, event, BOMBERS_NAME);
        Call(ds, event, SPRITE_RESTORE, hx, row);
        break;
      }
      opeu_air_strike_hit(ds, event, x, y, planes);
    }
    Call(ds, event, DELAY, 2);
    Call(ds, event, SPRITE_RESTORE, hx, row);
  }
  data.map_mode[0] = view;
  if (planes) {
    Call(ds, event, OFFICER_GAIN, side, AIR_OFFICER_KIND);
    record.pending_bombers = static_cast<std::uint16_t>(record.pending_bombers + planes);
  }
  record.strike.planes = 0;
  Call(ds, event, FINISH_SIDE, 0);
  Call(ds, event, MUSIC_COMMAND7);
  Call(ds, event, MUSIC_STOP);
  Call(ds, event, TURN_PRE_CONTEXT);
}

uint16_t svc_airstrk(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t   result{ 0 };
  auto const byte{ [&](unsigned i) { return static_cast<std::uint8_t>(Arg(args, count, i)); } };
  switch (id) {
  case OFFICER_RATING: result = opeu_officer_rating(ds, byte(0), byte(1)); break;
  case NIGHT:          result = opeu_hour_is_night(byte(0)); break;
  case UNIT_AT:        result = opeu_find_unit_at(ds, byte(0), byte(1)); break;
  case UNIT_SIDE:      result = opeu_unit_side(Arg(args, count, 0)); break;
  case OBJECTIVE_AT:   result = opeu_find_objective_at(ds, byte(0), byte(1)); break;
  case OBJECTIVE_SIDE: result = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case REQUEST_FORMAT: opeu_request_format(ds, { args, count }); break;
  case SIDE_RECORD:    result = opeu_find_side_record(ds, byte(0), byte(1)); break;
  case MESSAGE:        opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case AIR_REPORT:     {
    result = opeu_air_report(ds, svc_airshow, Arg(args, count, 1), byte(0));
    break;
  }
  case PERSON_TEMPLATE: result = opeu_find_person_template(ds, byte(0), byte(1)); break;
  case OPPOSITE_SIDE:   result = opeu_opposite_side(byte(0)); break;
  case SOUND_CUE:       opeu_sound_cue4(ds, svc_soundcue); break;
  case MAP_CODE:        result = opeu_map_code(ds, byte(0), byte(1)); break;
  case ISQRT:           result = opeu_isqrt(Arg(args, count, 0)); break;
  case CASUALTIES:      {
    opeu_unit_casualties(ds, svc_casualty, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
    break;
  }
  case OBJECTIVE_DAMAGE: {
    opeu_objective_damage(ds, svc_objdamag, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5), Arg(args, count, 6));
    break;
  }
  case TURN_CONTEXT:    opeu_set_turn_context(ds, svc_sound, byte(0)); break;
  case SIDE_FOR_STATUS: result = opeu_side_for_status(ds, byte(0)); break;
  case MAP_CENTER:      {
    opeu_map_center(ds, svc_mappos, byte(0), byte(1), byte(2));
    break;
  }
  case VIEW_PIXEL:  opeu_view_pixel(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case SPRITE_DRAW: {
    opeu_air_sprite_draw(ds, svc_airshow, 0, Arg(args, count, 0), Arg(args, count, 1), byte(2));
    break;
  }
  case DAILY_SHOW:       opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), byte(1), static_cast<std::uint32_t>(Arg(args, count, 3)) << 16 | Arg(args, count, 2)); break;
  case MUSIC_COMMAND7:   opeu_music_command7(ds, svc_sound); break;
  case DELAY:            opeu_delay_ticks(ds, svc_waitkey, Arg(args, count, 0)); break;
  case SPRITE_RESTORE:   opeu_air_sprite_restore(ds, svc_airshow, 0, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OFFICER_GAIN:     opeu_officer_gain(ds, byte(0), byte(1)); break;
  case FINISH_SIDE:      opeu_finish_side_initialization(ds, svc_sound, byte(0)); break;
  case MUSIC_STOP:       opeu_music_stop(ds, svc_sound); break;
  case TURN_PRE_CONTEXT: opeu_turn_pre_context(ds, svc_context); break;
  default:               port_unhandled("airstrk", id, count);
  }
  return result;
}

auto port_routine_opeu_air_strike_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_air_strike_present(ds, svc_airstrk);
  return 0;
}
