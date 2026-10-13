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
#include "common/game.hpp"
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
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/mappixel.hpp"
#include "map/mappos.hpp"
#include "map/query.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/unitname.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

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
auto Report(Game& game, std::uint16_t text, std::uint16_t format, std::uint16_t color, std::uint16_t name, std::uint16_t const (&now)[3], std::uint16_t const (&before)[3]) -> void {
  opeu_request_format(game, std::array<std::uint16_t, 10>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(color), static_cast<std::uint16_t>(name),
                                                           static_cast<std::uint16_t>(now[1]), static_cast<std::uint16_t>(static_cast<std::uint16_t>(now[1] - before[1])),
                                                           static_cast<std::uint16_t>(now[0]), static_cast<std::uint16_t>(static_cast<std::uint16_t>(now[0] - before[0])),
                                                           static_cast<std::uint16_t>(now[2]), static_cast<std::uint16_t>(static_cast<std::uint16_t>(now[2] - before[2])) });
}
} // namespace

// Counts the air side's airlift down an hour; when its delay runs out the air officer decides: 1 when it flies now,
// else it waits another hour or is cancelled.
uint16_t opeu_cargo_drop_due(Game& game) {
  auto& lift{ AirSide(Data(game)).lift };
  if (!lift.planes) return 0;
  opeu_sub_byte(1, &lift.turns);
  if (!opeu_sub_byte(1, &lift.hours)) switch (static_cast<std::uint8_t>(opeu_air_report(game, CARGO_NAME, static_cast<std::uint8_t>(lift.turns)))) {
    case 0: return 1;
    case 1: lift.hours = 1; break;
    case 2: lift.planes = 0; break;
    }
  return 0;
}

// Takes `amount` of the load at `cargo` (an AirLift), at random from the three supplies, until it is taken or the
// load is empty.
void opeu_cargo_distribute(Game& game, uint16_t cargo, uint16_t amount) {
  auto&         lift{ Record<AirLift>(Data(game), cargo) };
  std::uint16_t load[3]{ lift.supplies[0], lift.supplies[1], lift.supplies[2] };
  while (amount != 0 && (load[2] | load[1] | load[0]) != 0)
    for (auto& supply : load) {
      auto const taken{ opeu_min16(RandomBelow(game, static_cast<std::uint16_t>(amount + 1)), supply) };
      opeu_sub_word(taken, &supply);
      amount = static_cast<std::uint16_t>(amount - taken);
    }
  for (int i{ 0 }; i < 3; ++i) lift.supplies[i] = load[i];
}

// The daily-show presenter of an airlift: the side's air officer announces it, or the enemy's sees it coming.
void opeu_cargo_drop_present(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ opeu_find_side_record(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(FLIGHT_COMMANDER_KIND)) };
    auto const officer{ opeu_find_person_template(game, static_cast<std::uint8_t>(Record<Person>(data, commander).status), static_cast<std::uint8_t>(AIR_OFFICER_TEMPLATE)) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), CARGO_BEGIN, officer);
  } else {
    auto const enemy{ static_cast<std::uint8_t>(opeu_opposite_side(static_cast<std::uint8_t>(side))) };
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), CARGO_SPOTTED,
                         opeu_find_side_record(game, static_cast<std::uint8_t>(enemy), static_cast<std::uint8_t>(AIR_OFFICER_KIND)));
  }
  opeu_sound_cue4(game);
}

// The airlift flies (see the file comment).
void opeu_cargo_drop_event(Game& game) {
  ServiceLocals const locals{ DROP_LOCALS };
  auto&               data{ Data(game) };
  auto const          side{ data.air_side };
  auto&               record{ data.sides[side] };
  auto&               lift{ record.lift };
  auto const          x{ lift.x };
  auto const          y{ lift.y };
  auto                planes{ lift.planes };
  opeu_sub_word(planes, &record.planes);
  for (int i{ 0 }; i < 3; ++i) opeu_sub_word(lift.supplies[i], &record.stock[i]);
  opeu_sub_word(opeu_air_damage_roll(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(opeu_min16(planes, 0x1d) / 10)), &planes);
  opeu_cargo_distribute(game, Near(data, lift), static_cast<std::uint16_t>((lift.planes - planes) * LOAD_PER_PLANE));
  auto const view{ data.map_mode[0] };
  data.map_mode[0] = 1;
  opeu_map_center(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), static_cast<std::uint8_t>(static_cast<std::uint8_t>(opeu_side_for_status(game, static_cast<std::uint8_t>(side)))));
  auto const pixel{ CellViewPixel(game, x, y) };
  auto const text{ locals.At(TEXT_AT) };
  for (std::uint16_t row{ FLIGHT_TOP_ROW }; row >= FLIGHT_BOTTOM_ROW; --row) {
    if (row <= LOST_ROW && planes == 0) {
      opeu_air_destroyed_message(game, PLANES_NAME);
      break;
    }
    opeu_air_sprite_draw(game, 0, pixel.x, row, static_cast<std::uint8_t>(side));
    if (pixel.y == row) {
      auto const shown{ static_cast<std::uint8_t>(opeu_side_for_status(game, static_cast<std::uint8_t>(side))) };
      opeu_daily_show(game, static_cast<const DialogHost*>(port_dialogs_host()), CARGO_PICTURE, static_cast<std::uint8_t>(shown), static_cast<std::uint32_t>(CARGO_SEGMENT) << 16 | CARGO_PRESENTER);
    }
    if (static_cast<std::uint16_t>(pixel.y - row) == DROP_LEAD) {
      opeu_music_command7(game);
      if (auto const near{ opeu_find_unit_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) }; near != 0) {
        opeu_unit_capacities(game, near, port_ds_word_ptr(game, locals.At(AMMO_CAPACITY_AT)), port_ds_word_ptr(game, locals.At(FUEL_CAPACITY_AT)), port_ds_word_ptr(game, locals.At(FOOD_CAPACITY_AT)));
        auto&               unit{ Record<Unit>(data, near) };
        std::uint16_t const before[3]{ unit.resources[0], unit.resources[1], unit.resources[2] };
        std::uint16_t const limit[3]{ Word(game, locals.At(AMMO_CAPACITY_AT)), Word(game, locals.At(FUEL_CAPACITY_AT)), Word(game, locals.At(FOOD_CAPACITY_AT)) };
        Unload(lift, unit.resources, limit);
        if (static_cast<std::uint8_t>(opeu_unit_side(near)) == side) {
          std::uint16_t const now[3]{ unit.resources[0], unit.resources[1], unit.resources[2] };
          auto const          name{ opeu_unit_name(game, near, locals.At(NAME_AT)) };
          auto const          color{ SideColorCode(static_cast<std::uint8_t>(opeu_unit_side(near))) };
          Report(game, text, UNIT_REPORT, color, name, now, before);
          opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text,
                               opeu_find_side_record(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(AIR_OFFICER_KIND)));
        }
      }
      if (auto const near{ opeu_find_objective_at(game, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y)) }; near != 0) {
        auto&                   objective{ Record<Objective>(data, near) };
        std::uint16_t const     before[3]{ objective.resources[0], objective.resources[1], objective.resources[2] };
        constexpr std::uint16_t LIMIT[3]{ STOCK_LIMIT, STOCK_LIMIT, STOCK_LIMIT };
        Unload(lift, objective.resources, LIMIT);
        if (static_cast<std::uint8_t>(opeu_objective_side(game, near)) == side) {
          std::uint16_t const now[3]{ objective.resources[0], objective.resources[1], objective.resources[2] };
          auto const          color{ SideColorCode(static_cast<std::uint8_t>(opeu_objective_side(game, near))) };
          Report(game, text, CITY_REPORT, color, near, now, before);
          opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), text,
                               opeu_find_side_record(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(AIR_OFFICER_KIND)));
        }
      }
    }
    opeu_delay_ticks(game, 1);
    opeu_air_sprite_restore(game, 0, pixel.x, row);
  }
  data.map_mode[0] = view;
  for (int i{ 0 }; i < 3; ++i) opeu_add_word(lift.supplies[i], STOCK_LIMIT, &record.stock[i]);
  if (planes != 0) {
    opeu_officer_gain(game, static_cast<std::uint8_t>(side), static_cast<std::uint8_t>(AIR_OFFICER_KIND));
    record.pending_planes = static_cast<std::uint16_t>(record.pending_planes + planes);
  }
  lift.planes = 0;
}

// The hourly air operations of both sides: each side's strike, airborne drop and airlift that are due fly; a flown
// drop or airlift clears the side's ready flag for it.
void opeu_air_operations(Game& game) {
  auto& data{ Data(game) };
  for (data.air_side = 0; data.air_side < 2; ++data.air_side) {
    if (opeu_air_strike_due(game)) opeu_air_strike_event(game);
    if (opeu_para_drop_due(game)) {
      opeu_para_drop_event(game, 0);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_DROP_READY));
    }
    if (opeu_cargo_drop_due(game)) {
      opeu_cargo_drop_event(game);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_CARGO_READY));
    }
  }
}

auto port_routine_opeu_cargo_drop_present(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_cargo_drop_present(game);
  return 0;
}
