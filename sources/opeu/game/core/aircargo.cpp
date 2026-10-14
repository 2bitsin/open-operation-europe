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
#include "common/side-color.hpp"
#include "core/airshow.hpp"
#include "core/airstrk.hpp"
#include "core/dialogs.hpp"
#include "core/msnrept.hpp"
#include "core/paradrop.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/template.hpp"
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

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>

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

// opeu_cargo_drop_event's locals that services fill, as offsets in its reserve: the report and the unit's name.
constexpr std::uint16_t TEXT_AT{ 0x00 };     // 100 bytes
constexpr std::uint16_t NAME_AT{ 0x64 };     // 18 bytes
constexpr std::uint16_t DROP_LOCALS{ 0x7c }; // the reserve's size

// A unit's or objective's supplies before and after a drop.
struct Delivery {
  std::array<std::uint16_t, 3> before;
  std::array<std::uint16_t, 3> now;
};

// What the report on a delivery names: the text buffer it is written into, its format and the receiver's colour and name.
struct ReportHeading {
  TextTarget     text;
  std::uint16_t  format{};
  std::uint16_t  color{};
  FormatArgument name;
};

// Drops the load into `stock` up to `limit` each; what does not fit stays in the load. Returns the stock before and after.
auto Unload(AirLift& lift, std::span<std::uint16_t, 3> stock, std::span<std::uint16_t const, 3> limit) -> Delivery {
  Delivery delivery{};
  std::ranges::copy(stock, delivery.before.begin());
  for (auto&& [supply, stocked, most] : std::views::zip(lift.supplies, stock, limit)) {
    auto const before{ stocked };
    stocked = AddWord(stocked, supply, most);
    auto const after{ stocked };
    supply = static_cast<std::uint16_t>(supply + static_cast<std::uint16_t>(before - after));
  }
  std::ranges::copy(stock, delivery.now.begin());
  return delivery;
}

// The report on what a unit or objective received: colour, name, then each supply's new amount and gain (fuel,
// ammunition, food).
auto Report(Game& game, ReportHeading const& heading, Delivery const& delivery) -> void {
  auto const& [before, now]{ delivery };
  auto const gain{ [&](std::size_t supply) { return static_cast<std::uint16_t>(now[supply] - before[supply]); } };
  opeu_request_format(game, heading.text, std::array<FormatArgument, 9>{ heading.format, heading.color, heading.name, now[1], gain(1), now[0], gain(0), now[2], gain(2) });
}
} // namespace

// Counts the air side's airlift down an hour; when its delay runs out the air officer decides: 1 when it flies now,
// else it waits another hour or is cancelled.
uint16_t opeu_cargo_drop_due(Game& game) {
  auto& lift{ AirSide(Data(game)).lift };
  if (!lift.planes) return 0;
  lift.turns = SubByte(lift.turns, 1);
  lift.hours = SubByte(lift.hours, 1);
  if (!lift.hours) switch (opeu_air_report(game, CARGO_NAME, lift.turns)) {
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
      auto const taken{ opeu_min16(game.State().RandomBounded(static_cast<std::uint16_t>(amount + 1)), supply) };
      supply = SubWord(supply, taken);
      amount = static_cast<std::uint16_t>(amount - taken);
    }
  for (int i{ 0 }; i < 3; ++i) lift.supplies[i] = load[i];
}

// The daily-show presenter of an airlift: the side's air officer announces it, or the enemy's sees it coming.
void opeu_cargo_drop_present(Game& game) {
  auto&      data{ Data(game) };
  auto const side{ data.air_side };
  if (data.side_status[side] == SIDE_PLAYED) {
    auto const commander{ opeu_find_side_record(game, side, FLIGHT_COMMANDER_KIND) };
    auto const officer{ opeu_find_person_template(game, Record<Person>(data, commander).status, AIR_OFFICER_TEMPLATE) };
    opeu_control_message(game, CARGO_BEGIN, officer);
  } else {
    auto const enemy{ opeu_opposite_side(side) };
    opeu_control_message(game, CARGO_SPOTTED, opeu_find_side_record(game, enemy, AIR_OFFICER_KIND));
  }
  opeu_sound_cue4(game);
}

// The airlift flies (see the file comment).
void opeu_cargo_drop_event(Game& game) {
  TextBuffer<DROP_LOCALS> locals;
  auto&                   data{ Data(game) };
  auto const              side{ data.air_side };
  auto&                   record{ data.sides[side] };
  auto&                   lift{ record.lift };
  auto const              x{ lift.x };
  auto const              y{ lift.y };
  auto                    planes{ lift.planes };
  record.planes = SubWord(record.planes, planes);
  for (int i{ 0 }; i < 3; ++i) record.stock[i] = SubWord(record.stock[i], lift.supplies[i]);
  planes = SubWord(planes, opeu_air_damage_roll(game, x, y, opeu_min16(planes, 0x1d) / 10));
  opeu_cargo_distribute(game, Near(data, lift), (lift.planes - planes) * LOAD_PER_PLANE);
  auto const view{ data.map_mode[0] };
  data.map_mode[0] = 1;
  opeu_map_center(game, x, y, opeu_side_for_status(game, side));
  auto const pixel{ ViewPixel(Data(game), { static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y) }) };
  auto const text{ locals.At(TEXT_AT) };
  for (std::uint16_t row{ FLIGHT_TOP_ROW }; row >= FLIGHT_BOTTOM_ROW; --row) {
    if (row <= LOST_ROW && planes == 0) {
      opeu_air_destroyed_message(game, PLANES_NAME);
      break;
    }
    opeu_air_sprite_draw(game, 0, pixel.x, row, side);
    if (pixel.y == row) {
      auto const shown{ opeu_side_for_status(game, side) };
      opeu_daily_show(game, CARGO_PICTURE, shown, Presenter::CARGO_DROP);
    }
    if (static_cast<std::uint16_t>(pixel.y - row) == DROP_LEAD) {
      opeu_music_command7(game);
      if (auto const near{ opeu_find_unit_at(game, x, y) }; near != 0) {
        auto const limit{ Figures(UnitCapacities(data, near)) };
        auto const delivery{ Unload(lift, Record<Unit>(data, near).resources, limit) };
        if (opeu_unit_side(near) == side) {
          auto const name{ opeu_unit_name(game, near, locals.At(NAME_AT)) };
          auto const color{ SideColorCode(opeu_unit_side(near)) };
          Report(game, { text, UNIT_REPORT, color, name }, delivery);
          opeu_control_message(game, text, opeu_find_side_record(game, side, AIR_OFFICER_KIND));
        }
      }
      if (auto const near{ opeu_find_objective_at(game, x, y) }; near != 0) {
        constexpr std::array<std::uint16_t, 3> LIMIT{ STOCK_LIMIT, STOCK_LIMIT, STOCK_LIMIT };
        auto const                             delivery{ Unload(lift, Record<Objective>(data, near).resources, LIMIT) };
        if (opeu_objective_side(game, near) == side) {
          auto const color{ SideColorCode(opeu_objective_side(game, near)) };
          Report(game, { text, CITY_REPORT, color, near }, delivery);
          opeu_control_message(game, text, opeu_find_side_record(game, side, AIR_OFFICER_KIND));
        }
      }
    }
    opeu_delay_ticks(game, 1);
    opeu_air_sprite_restore(game, 0, pixel.x, row);
  }
  data.map_mode[0] = view;
  for (int i{ 0 }; i < 3; ++i) record.stock[i] = AddWord(record.stock[i], lift.supplies[i], STOCK_LIMIT);
  if (planes != 0) {
    opeu_officer_gain(game, side, AIR_OFFICER_KIND);
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
      opeu_para_drop_event(game);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_DROP_READY));
    }
    if (opeu_cargo_drop_due(game)) {
      opeu_cargo_drop_event(game);
      data.side_flags[data.air_side] &= static_cast<std::uint8_t>(~Bits(SideFlag::AIR_CARGO_READY));
    }
  }
}
