// The supply airlift's flight, and the hourly air operations. A side's airlift order (Side::lift: target cell, the
// ammunition, fuel and food carried, the transport planes, the hours until it flies and the hours it may wait) counts
// down each hour and asks the air officer whether it flies now, waits or is cancelled. When it flies the load leaves
// the side's stock, the air defence shoots planes down (their share of the load is lost), the transport flies down
// the screen and drops the load into the unit and the objective under it (each up to its capacity) with a report to
// the owner; what is left and the surviving planes return to the side.
#include "core/aircargo.hpp"
#include "common/air-flight.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "common/side-color.hpp"
#include "core/airshow.hpp"
#include "core/airstrk.hpp"
#include "core/dialogs.hpp"
#include "core/msnrept.hpp"
#include "core/paradrop.hpp"
#include "core/people.hpp"
#include "core/pixel-service.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
#include "glue.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/unitname.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the air operations call through their event.
enum Service : std::uint16_t {
  AIR_REPORT        = 0,  // (mission name, hours left) -> 0 fly now, 1 wait an hour, 2 cancel
  DAMAGE_ROLL       = 1,  // (x, y, waves) -> planes shot down
  DESTROYED_MESSAGE = 2,  // (mission name)
  SPRITE_DRAW       = 3,  // (x, y, side)
  SPRITE_RESTORE    = 4,  // (x, y)
  STRIKE_DUE        = 5,  // ()
  STRIKE_EVENT      = 6,  // ()
  PARA_DUE          = 7,  // ()
  PARA_EVENT        = 8,  // ()
  SIDE_RECORD       = 9,  // (side, rank) -> person
  PERSON_TEMPLATE   = 10, // (nation, kind) -> person template
  OPPOSITE_SIDE     = 11, // (side)
  SIDE_FOR_STATUS   = 12, // (side)
  MESSAGE           = 13, // (text, person)
  SOUND_CUE         = 14, // ()
  REQUEST_FORMAT    = 15, // (destination, format, words...)
  MAP_CENTER        = 16, // (x, y, side)
  VIEW_PIXEL        = 17, // (&x, &y): a cell to its screen pixel
  DAILY_SHOW        = 18, // (picture, side, presenter offset, presenter segment)
  MUSIC_COMMAND7    = 19, // ()
  DELAY             = 20, // (ticks)
  UNIT_AT           = 21, // (x, y) -> unit or 0
  OBJECTIVE_AT      = 22, // (x, y) -> objective or 0
  UNIT_SIDE         = 23, // (unit)
  OBJECTIVE_SIDE    = 24, // (objective)
  CAPACITIES        = 25, // (unit, &ammunition, &fuel, &food)
  UNIT_NAME         = 26, // (unit, buffer) -> the name
  OFFICER_GAIN      = 27, // (side, kind)
};

constexpr std::uint16_t CARGO_NAME{ offsetof(DataSegment, cargo_drop_name) };
constexpr std::uint16_t CARGO_BEGIN{ offsetof(DataSegment, cargo_drop_begin) };
constexpr std::uint16_t CARGO_SPOTTED{ offsetof(DataSegment, cargo_drop_spotted) };
constexpr std::uint16_t PLANES_NAME{ offsetof(DataSegment, cargo_planes_name) };
constexpr std::uint16_t UNIT_REPORT{ offsetof(DataSegment, cargo_unit_report) };
constexpr std::uint16_t CITY_REPORT{ offsetof(DataSegment, cargo_city_report) };

constexpr std::uint16_t LOAD_PER_PLANE{ 3000 };
constexpr std::uint16_t STOCK_LIMIT{ 60000 };
constexpr std::uint16_t LOST_ROW{ 0x114 }; // with no plane left the flight ends here
constexpr std::uint16_t DROP_LEAD{ 0x20 }; // rows above the target where the load drops
constexpr std::uint16_t CARGO_PICTURE{ 0x103 };
constexpr std::uint16_t CARGO_PRESENTER{ 0x1042 }; // opeu_cargo_drop_present, in the code segment below
constexpr std::uint16_t CARGO_SEGMENT{ 0x0f43 };

// opeu_cargo_drop_event's locals that services fill, as offsets in its reserve: the report, the unit's name and the
// unit's capacities.
constexpr std::uint16_t TEXT_AT{ 0x00 }; // 100 bytes
constexpr std::uint16_t NAME_AT{ 0x64 }; // 18 bytes
constexpr std::uint16_t AMMO_CAPACITY_AT{ 0x76 };
constexpr std::uint16_t FUEL_CAPACITY_AT{ 0x78 };
constexpr std::uint16_t FOOD_CAPACITY_AT{ 0x7a };
constexpr std::uint16_t DROP_LOCALS{ 0x7c };

auto AirSide(DataSegment& data) -> Side& {
  return data.sides[data.air_side];
}

// Drops the load into `stock` up to `limit` each; what does not fit stays in the load. Returns the stock before.
auto Unload(AirLift& lift, std::uint16_t (&stock)[3], std::uint16_t const (&limit)[3]) -> void {
  for (int i{ 0 }; i < 3; ++i) {
    auto const before{ stock[i] };
    auto const after{ opeu_add_word(lift.supplies[i], limit[i], &stock[i]) };
    lift.supplies[i] = static_cast<std::uint16_t>(lift.supplies[i] + static_cast<std::uint16_t>(before - after));
  }
}

// The report on what a unit or objective received: color, name, then each supply's new and old amount (fuel,
// ammunition, food).
auto Report(std::uint8_t* ds, HostEvent event, std::uint16_t text, std::uint16_t format, std::uint16_t color, std::uint16_t name, std::uint16_t const (&now)[3], std::uint16_t const (&before)[3])
    -> void {
  Call(ds, event, REQUEST_FORMAT, text, format, color, name, now[1], static_cast<std::uint16_t>(now[1] - before[1]), now[0], static_cast<std::uint16_t>(now[0] - before[0]), now[2],
       static_cast<std::uint16_t>(now[2] - before[2]));
}
} // namespace

// Counts the air side's airlift down an hour; when its delay runs out the air officer decides: 1 when it flies now,
// else it waits another hour or is cancelled.
uint16_t opeu_cargo_drop_due(uint8_t ds[65536], HostEvent event) {
  auto& lift{ AirSide(Data(ds)).lift };
  if (!lift.planes) return 0;
  opeu_sub_byte(1, &lift.turns);
  if (!opeu_sub_byte(1, &lift.hours)) switch (static_cast<std::uint8_t>(Call(ds, event, AIR_REPORT, CARGO_NAME, lift.turns))) {
    case 0: return 1;
    case 1: lift.hours = 1; break;
    case 2: lift.planes = 0; break;
    }
  return 0;
}

// Takes `amount` of the load at `cargo` (an AirLift), at random from the three supplies, until it is taken or the
// load is empty.
void opeu_cargo_distribute(uint8_t ds[65536], uint16_t cargo, uint16_t amount) {
  auto&         lift{ Record<AirLift>(Data(ds), cargo) };
  std::uint16_t load[3]{ lift.supplies[0], lift.supplies[1], lift.supplies[2] };
  while (amount != 0 && (load[2] | load[1] | load[0]) != 0)
    for (auto& supply : load) {
      auto const taken{ opeu_min16(RandomBelow(ds, static_cast<std::uint16_t>(amount + 1)), supply) };
      opeu_sub_word(taken, &supply);
      amount = static_cast<std::uint16_t>(amount - taken);
    }
  for (int i{ 0 }; i < 3; ++i) lift.supplies[i] = load[i];
}

// The daily-show presenter of an airlift: the side's air officer announces it, or the enemy's sees it coming.
void opeu_cargo_drop_present(uint8_t ds[65536], HostEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ Call(ds, event, SIDE_RECORD, side, FLIGHT_COMMANDER_KIND) };
    auto const officer{ Call(ds, event, PERSON_TEMPLATE, Record<Person>(data, commander).status, AIR_OFFICER_TEMPLATE) };
    Call(ds, event, MESSAGE, CARGO_BEGIN, officer);
  } else {
    auto const enemy{ static_cast<std::uint8_t>(Call(ds, event, OPPOSITE_SIDE, side)) };
    Call(ds, event, MESSAGE, CARGO_SPOTTED, Call(ds, event, SIDE_RECORD, enemy, AIR_OFFICER_KIND));
  }
  Call(ds, event, SOUND_CUE);
}

// The airlift flies (see the file comment).
void opeu_cargo_drop_event(uint8_t ds[65536], HostEvent event) {
  ServiceLocals const locals{ DROP_LOCALS };
  auto&               data{ Data(ds) };
  auto const          side{ data.air_side };
  auto&               record{ data.sides[side] };
  auto&               lift{ record.lift };
  auto const          x{ lift.x };
  auto const          y{ lift.y };
  auto                planes{ lift.planes };
  opeu_sub_word(planes, &record.planes);
  for (int i{ 0 }; i < 3; ++i) opeu_sub_word(lift.supplies[i], &record.stock[i]);
  opeu_sub_word(Call(ds, event, DAMAGE_ROLL, x, y, opeu_min16(planes, 0x1d) / 10), &planes);
  opeu_cargo_distribute(ds, Near(data, lift), static_cast<std::uint16_t>((lift.planes - planes) * LOAD_PER_PLANE));
  auto const view{ data.map_mode[0] };
  data.map_mode[0] = 1;
  Call(ds, event, MAP_CENTER, x, y, static_cast<std::uint8_t>(Call(ds, event, SIDE_FOR_STATUS, side)));
  auto const pixel{ ServicePixel(ds, event, VIEW_PIXEL, x, y) };
  auto const text{ locals.At(TEXT_AT) };
  for (std::uint16_t row{ FLIGHT_TOP_ROW }; row >= FLIGHT_BOTTOM_ROW; --row) {
    if (row <= LOST_ROW && planes == 0) {
      Call(ds, event, DESTROYED_MESSAGE, PLANES_NAME);
      break;
    }
    Call(ds, event, SPRITE_DRAW, pixel.x, row, side);
    if (pixel.y == row) {
      auto const shown{ static_cast<std::uint8_t>(Call(ds, event, SIDE_FOR_STATUS, side)) };
      Call(ds, event, DAILY_SHOW, CARGO_PICTURE, shown, CARGO_PRESENTER, CARGO_SEGMENT);
    }
    if (static_cast<std::uint16_t>(pixel.y - row) == DROP_LEAD) {
      Call(ds, event, MUSIC_COMMAND7);
      if (auto const near{ Call(ds, event, UNIT_AT, x, y) }; near != 0) {
        Call(ds, event, CAPACITIES, near, locals.At(AMMO_CAPACITY_AT), locals.At(FUEL_CAPACITY_AT), locals.At(FOOD_CAPACITY_AT));
        auto&               unit{ Record<Unit>(data, near) };
        std::uint16_t const before[3]{ unit.resources[0], unit.resources[1], unit.resources[2] };
        std::uint16_t const limit[3]{ Word(ds, locals.At(AMMO_CAPACITY_AT)), Word(ds, locals.At(FUEL_CAPACITY_AT)), Word(ds, locals.At(FOOD_CAPACITY_AT)) };
        Unload(lift, unit.resources, limit);
        if (static_cast<std::uint8_t>(Call(ds, event, UNIT_SIDE, near)) == side) {
          std::uint16_t const now[3]{ unit.resources[0], unit.resources[1], unit.resources[2] };
          auto const          name{ Call(ds, event, UNIT_NAME, near, locals.At(NAME_AT)) };
          auto const          color{ SideColorCode(static_cast<std::uint8_t>(Call(ds, event, UNIT_SIDE, near))) };
          Report(ds, event, text, UNIT_REPORT, color, name, now, before);
          Call(ds, event, MESSAGE, text, Call(ds, event, SIDE_RECORD, side, AIR_OFFICER_KIND));
        }
      }
      if (auto const near{ Call(ds, event, OBJECTIVE_AT, x, y) }; near != 0) {
        auto&                   objective{ Record<Objective>(data, near) };
        std::uint16_t const     before[3]{ objective.resources[0], objective.resources[1], objective.resources[2] };
        constexpr std::uint16_t LIMIT[3]{ STOCK_LIMIT, STOCK_LIMIT, STOCK_LIMIT };
        Unload(lift, objective.resources, LIMIT);
        if (static_cast<std::uint8_t>(Call(ds, event, OBJECTIVE_SIDE, near)) == side) {
          std::uint16_t const now[3]{ objective.resources[0], objective.resources[1], objective.resources[2] };
          auto const          color{ SideColorCode(static_cast<std::uint8_t>(Call(ds, event, OBJECTIVE_SIDE, near))) };
          Report(ds, event, text, CITY_REPORT, color, near, now, before);
          Call(ds, event, MESSAGE, text, Call(ds, event, SIDE_RECORD, side, AIR_OFFICER_KIND));
        }
      }
    }
    Call(ds, event, DELAY, 1);
    Call(ds, event, SPRITE_RESTORE, pixel.x, row);
  }
  data.map_mode[0] = view;
  for (int i{ 0 }; i < 3; ++i) opeu_add_word(lift.supplies[i], STOCK_LIMIT, &record.stock[i]);
  if (planes != 0) {
    Call(ds, event, OFFICER_GAIN, side, AIR_OFFICER_KIND);
    record.pending_planes = static_cast<std::uint16_t>(record.pending_planes + planes);
  }
  lift.planes = 0;
}

// The hourly air operations of both sides: each side's strike, airborne drop and airlift that are due fly; a flown
// drop or airlift clears the side's ready flag for it.
void opeu_air_operations(uint8_t ds[65536], HostEvent event) {
  auto& data{ Data(ds) };
  for (data.air_side = 0; data.air_side < 2; ++data.air_side) {
    if (Call(ds, event, STRIKE_DUE)) Call(ds, event, STRIKE_EVENT);
    if (Call(ds, event, PARA_DUE)) {
      Call(ds, event, PARA_EVENT);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_DROP_READY));
    }
    if (opeu_cargo_drop_due(ds, event)) {
      opeu_cargo_drop_event(ds, event);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_CARGO_READY));
    }
  }
}

uint16_t svc_aircargo(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t   result{ 0 };
  auto const byte{ [&](unsigned i) { return static_cast<std::uint8_t>(Arg(args, count, i)); } };
  switch (id) {
  case AIR_REPORT: {
    result = opeu_air_report(ds, svc_airshow, Arg(args, count, 0), byte(1));
    break;
  }
  case DAMAGE_ROLL:       result = opeu_air_damage_roll(ds, svc_airstrk, byte(0), byte(1), byte(2)); break;
  case DESTROYED_MESSAGE: {
    opeu_air_destroyed_message(ds, svc_airstrk, Arg(args, count, 0));
    break;
  }
  case SPRITE_DRAW: {
    opeu_air_sprite_draw(ds, svc_airshow, 0, Arg(args, count, 0), Arg(args, count, 1), byte(2));
    break;
  }
  case SPRITE_RESTORE: opeu_air_sprite_restore(ds, svc_airshow, 0, Arg(args, count, 0), Arg(args, count, 1)); break;
  case STRIKE_DUE:     result = opeu_air_strike_due(ds, svc_airstrk); break;
  case STRIKE_EVENT:   {
    opeu_air_strike_event(ds, svc_airstrk);
    break;
  }
  case PARA_DUE:   result = opeu_para_drop_due(ds, svc_paradrop); break;
  case PARA_EVENT: {
    opeu_para_drop_event(ds, svc_paradrop, 0);
    break;
  }
  case SIDE_RECORD:     result = opeu_find_side_record(ds, byte(0), byte(1)); break;
  case PERSON_TEMPLATE: result = opeu_find_person_template(ds, byte(0), byte(1)); break;
  case OPPOSITE_SIDE:   result = opeu_opposite_side(byte(0)); break;
  case SIDE_FOR_STATUS: result = opeu_side_for_status(ds, byte(0)); break;
  case MESSAGE:         opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case SOUND_CUE:       opeu_sound_cue4(ds, svc_soundcue); break;
  case REQUEST_FORMAT:  opeu_request_format(ds, { args, count }); break;
  case MAP_CENTER:      {
    opeu_map_center(ds, svc_mappos, byte(0), byte(1), byte(2));
    break;
  }
  case VIEW_PIXEL:     opeu_view_pixel(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case DAILY_SHOW:     opeu_daily_show(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), byte(1), static_cast<std::uint32_t>(Arg(args, count, 3)) << 16 | Arg(args, count, 2)); break;
  case MUSIC_COMMAND7: opeu_music_command7(ds, svc_sound); break;
  case DELAY:          opeu_delay_ticks(ds, svc_waitkey, Arg(args, count, 0)); break;
  case UNIT_AT:        result = opeu_find_unit_at(ds, byte(0), byte(1)); break;
  case OBJECTIVE_AT:   result = opeu_find_objective_at(ds, byte(0), byte(1)); break;
  case UNIT_SIDE:      result = opeu_unit_side(Arg(args, count, 0)); break;
  case OBJECTIVE_SIDE: result = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case CAPACITIES:     opeu_unit_capacities(ds, Arg(args, count, 0), (uint16_t*)PORT_DS(Arg(args, count, 1)), (uint16_t*)PORT_DS(Arg(args, count, 2)), (uint16_t*)PORT_DS(Arg(args, count, 3))); break;
  case UNIT_NAME:      result = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case OFFICER_GAIN:   opeu_officer_gain(ds, byte(0), byte(1)); break;
  default:             port_unhandled("aircargo", id, count);
  }
  return result;
}

auto port_routine_opeu_cargo_drop_present(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_cargo_drop_present(ds, svc_aircargo);
  return 0;
}
