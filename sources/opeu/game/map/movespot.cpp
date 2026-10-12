// Spotting while the current unit moves: it spots enemy elements within sight (the weather's range, halved at night),
// and the nearest selectable enemy unit may spot its elements in turn. Also: whether a spotted enemy unit stands next
// to the current unit, and the end of the minefield report.
#include "types.hpp"
#include "map/movespot.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "core/sideutil.hpp"
#include "io/waitkey.hpp"
#include "map/distance.hpp"
#include "map/movesup.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "glue.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {
enum class SpotService : uint16_t {
  OPPOSITE_SIDE  = 3,  // side
  IS_NIGHT       = 4,  // hour
  RANDOM         = 5,  // bound
  UNIT_SPOTTED   = 6,  // unit
  UNIT_ELIGIBLE  = 7,  // unit
  GRID_DISTANCE  = 8,  // x, y, x2, y2
  GRID_STEP      = 9,  // DS offsets of x and y, direction
  CELL_SIDE      = 10, // x, y
  FIND_UNIT      = 11, // x, y
  PENDING_FINISH = 12, // request
};
constexpr uint8_t NONE{ 255 };
constexpr uint8_t DIRECTIONS{ 8 };
constexpr uint8_t MINES_REQUEST{
  3
}; // the pending request the minefield report finishes
constexpr uint16_t STEP_X_AT{ 2 }; // the step coordinates live below the frame
constexpr uint16_t STEP_Y_AT{ 4 };
} // namespace

void opeu_move_spot(uint8_t* ds, HostEvent event) {
  auto&      data{ Data(ds) };
  auto const current{ data.current_unit_ptr };
  auto&      mover{ Record<Unit>(data, current) };
  auto const x{ mover.x };
  auto const y{ mover.y };
  auto const enemy{ static_cast<uint8_t>(
      Call(ds, event, SpotService::OPPOSITE_SIDE, data.current_side)) };
  auto const sight{ static_cast<uint8_t>(
      data.sight_ranges[data.weather] /
      (Call(ds, event, SpotService::IS_NIGHT, data.hour) ? 2 : 1)) };
  uint8_t    nearest{ NONE };
  uint16_t   spotter{ 0 };
  for (uint8_t i{ 0 }; i < UNITS_PER_SIDE; i++) {
    auto& other{ data.units[enemy * UNITS_PER_SIDE + i] };
    if (!(other.flags & Bits(UnitFlag::READY)))
      continue;
    auto const unit{ Near(data, other) };
    auto const distance{ static_cast<uint8_t>(
        Call(ds, event, SpotService::GRID_DISTANCE, x, y, other.x, other.y)) };
    if (Call(ds, event, SpotService::UNIT_ELIGIBLE, current) &&
        sight > distance) {
      auto const was_spotted{ Call(ds, event, SpotService::UNIT_SPOTTED,
                                   unit) };
      for (auto& element : Elements(data, other))
        if (Call(ds, event, SpotService::RANDOM, sight) >= distance)
          element.flags |= Bits(ElementFlag::SPOTTED);
      if (Call(ds, event, SpotService::UNIT_SPOTTED, unit) != was_spotted &&
          data.side_status[data.current_side] == SIDE_PLAYED)
        opeu_move_sighted(ds, current, unit);
    }
    if (Call(ds, event, SpotService::UNIT_ELIGIBLE, unit) &&
        distance < nearest) {
      nearest = distance;
      spotter = unit;
    }
  }
  if (nearest == NONE || (mover.flags & Bits(UnitFlag::SPOTTED)))
    return;
  auto const was_spotted{ Call(ds, event, SpotService::UNIT_SPOTTED, current) };
  for (auto& element : Elements(data, mover)) {
    if (Call(ds, event, SpotService::RANDOM, sight) >= nearest)
      element.flags |= Bits(ElementFlag::SPOTTED);
    else if (!Call(ds, event, SpotService::RANDOM, sight * 10))
      element.flags &= ~Bits(ElementFlag::SPOTTED);
  }
  if (!was_spotted &&
      Call(ds, event, SpotService::UNIT_SPOTTED, current) != was_spotted &&
      data.side_status[enemy] == SIDE_PLAYED)
    opeu_move_sighted(ds, spotter, current);
}

// bp: this routine's frame; the step coordinates handed to the grid step live below it.
uint16_t opeu_move_enemy_adjacent(uint8_t* ds, HostEvent event, uint16_t bp) {
  auto const& data{ Data(ds) };
  auto const& mover{ Record<Unit>(data, data.current_unit_ptr) };
  auto const  x_at{ Local(bp, STEP_X_AT) };
  auto const  y_at{ Local(bp, STEP_Y_AT) };
  auto const  x{ mover.x };
  auto const  y{ mover.y };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS; direction++) {
    Byte(ds, x_at) = x;
    Byte(ds, y_at) = y;
    if (!Call(ds, event, SpotService::GRID_STEP, x_at, y_at, direction))
      continue;
    auto const side{ Call(ds, event, SpotService::CELL_SIDE, Byte(ds, x_at),
                          Byte(ds, y_at)) };
    if (static_cast<uint8_t>(
            Call(ds, event, SpotService::OPPOSITE_SIDE, data.current_side)) !=
        static_cast<uint8_t>(side))
      continue;
    if (Call(ds, event, SpotService::UNIT_SPOTTED,
             Call(ds, event, SpotService::FIND_UNIT, Byte(ds, x_at),
                  Byte(ds, y_at))))
      return 1;
  }
  return 0;
}

void opeu_move_mines_finish(uint8_t* ds, HostEvent event) {
  Call(ds, event, SpotService::PENDING_FINISH, MINES_REQUEST);
}

uint16_t svc_movespot(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) {
    return static_cast<uint8_t>(Arg(args, count, i));
  } };
  switch (static_cast<SpotService>(id)) {
    case SpotService::OPPOSITE_SIDE: return opeu_opposite_side(byte(0));
    case SpotService::IS_NIGHT:      return opeu_hour_is_night(byte(0));
    case SpotService::RANDOM:        return port_rand_bounded(ds, arg(0));
    case SpotService::UNIT_SPOTTED:  return opeu_unit_special(ds, arg(0));
    case SpotService::UNIT_ELIGIBLE:
      return opeu_unit_selection_eligible(ds, arg(0));
    case SpotService::GRID_DISTANCE:
      return opeu_grid_distance(byte(0), byte(1), byte(2), byte(3));
    case SpotService::GRID_STEP:
      return opeu_grid_step(PORT_DS(arg(0)), PORT_DS(arg(1)), byte(2));
    case SpotService::CELL_SIDE: return opeu_cell_side(ds, byte(0), byte(1));
    case SpotService::FIND_UNIT: return opeu_find_unit_at(ds, byte(0), byte(1));
    case SpotService::PENDING_FINISH:
      opeu_pending_finish(ds, svc_waitkey, byte(0));
      return 0;
    default: port_unhandled("movespot", id, count);
  }
  return 0;
}

auto port_routine_opeu_move_mines_finish(std::uint8_t*        ds,
                                         std::uint16_t const* args,
                                         unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_move_mines_finish(ds, svc_movespot);
  return 0;
}
