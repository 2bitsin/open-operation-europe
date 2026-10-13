// Spotting while the current unit moves: it spots enemy elements within sight (the weather's range, halved at night),
// and the nearest selectable enemy unit may spot its elements in turn. Also: whether a spotted enemy unit stands next
// to the current unit, and the end of the minefield report.
#include "map/movespot.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/sideutil.hpp"
#include "io/waitkey.hpp"
#include "map/distance.hpp"
#include "map/movesup.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
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
constexpr uint8_t MINES_REQUEST{ 3 }; // the pending request the minefield report finishes
// The DS reserve of the step coordinates the grid step service moves.
constexpr uint16_t STEP_RESERVE{ 4 };
constexpr uint16_t STEP_X_AT{ 2 };
constexpr uint16_t STEP_Y_AT{ 0 };
} // namespace

void opeu_move_spot(Game& game, HostEvent event) {
  auto&      data{ Data(game) };
  auto const current{ data.current_unit_ptr };
  auto&      mover{ Record<Unit>(data, current) };
  auto const x{ mover.x };
  auto const y{ mover.y };
  auto const enemy{ static_cast<uint8_t>(Call(game, event, SpotService::OPPOSITE_SIDE, data.current_side)) };
  auto const sight{ static_cast<uint8_t>(data.sight_ranges[data.weather] / (Call(game, event, SpotService::IS_NIGHT, data.hour) ? 2 : 1)) };
  uint8_t    nearest{ NONE };
  uint16_t   spotter{ 0 };
  for (uint8_t i{ 0 }; i < UNITS_PER_SIDE; i++) {
    auto& other{ data.units[enemy * UNITS_PER_SIDE + i] };
    if (!(other.flags & Bits(UnitFlag::READY))) continue;
    auto const unit{ Near(data, other) };
    auto const distance{ static_cast<uint8_t>(Call(game, event, SpotService::GRID_DISTANCE, x, y, other.x, other.y)) };
    if (Call(game, event, SpotService::UNIT_ELIGIBLE, current) && sight > distance) {
      auto const was_spotted{ Call(game, event, SpotService::UNIT_SPOTTED, unit) };
      for (auto& element : Elements(data, other))
        if (Call(game, event, SpotService::RANDOM, sight) >= distance) element.flags |= Bits(ElementFlag::SPOTTED);
      if (Call(game, event, SpotService::UNIT_SPOTTED, unit) != was_spotted && data.side_status[data.current_side] == SIDE_PLAYED) opeu_move_sighted(game, current, unit);
    }
    if (Call(game, event, SpotService::UNIT_ELIGIBLE, unit) && distance < nearest) {
      nearest = distance;
      spotter = unit;
    }
  }
  if (nearest == NONE || (mover.flags & Bits(UnitFlag::SPOTTED))) return;
  auto const was_spotted{ Call(game, event, SpotService::UNIT_SPOTTED, current) };
  for (auto& element : Elements(data, mover)) {
    if (Call(game, event, SpotService::RANDOM, sight) >= nearest)
      element.flags |= Bits(ElementFlag::SPOTTED);
    else if (!Call(game, event, SpotService::RANDOM, sight * 10))
      element.flags &= ~Bits(ElementFlag::SPOTTED);
  }
  if (!was_spotted && Call(game, event, SpotService::UNIT_SPOTTED, current) != was_spotted && data.side_status[enemy] == SIDE_PLAYED) opeu_move_sighted(game, spotter, current);
}

uint16_t opeu_move_enemy_adjacent(Game& game, HostEvent event) {
  auto const&         data{ Data(game) };
  auto const&         mover{ Record<Unit>(data, data.current_unit_ptr) };
  ServiceLocals const locals{ STEP_RESERVE };
  auto const          x_at{ locals.At(STEP_X_AT) };
  auto const          y_at{ locals.At(STEP_Y_AT) };
  auto const          x{ mover.x };
  auto const          y{ mover.y };
  for (uint8_t direction{ 0 }; direction < GRID_DIRECTIONS; direction++) {
    Byte(game, x_at) = x;
    Byte(game, y_at) = y;
    if (!Call(game, event, SpotService::GRID_STEP, x_at, y_at, direction)) continue;
    auto const side{ Call(game, event, SpotService::CELL_SIDE, Byte(game, x_at), Byte(game, y_at)) };
    if (static_cast<uint8_t>(Call(game, event, SpotService::OPPOSITE_SIDE, data.current_side)) != static_cast<uint8_t>(side)) continue;
    if (Call(game, event, SpotService::UNIT_SPOTTED, Call(game, event, SpotService::FIND_UNIT, Byte(game, x_at), Byte(game, y_at)))) return 1;
  }
  return 0;
}

void opeu_move_mines_finish(Game& game, HostEvent event) {
  Call(game, event, SpotService::PENDING_FINISH, MINES_REQUEST);
}

uint16_t svc_movespot(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  auto const arg{ [&](unsigned i) { return Arg(args, count, i); } };
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  switch (static_cast<SpotService>(id)) {
  case SpotService::OPPOSITE_SIDE:  return opeu_opposite_side(byte(0));
  case SpotService::IS_NIGHT:       return opeu_hour_is_night(byte(0));
  case SpotService::RANDOM:         return port_rand_bounded(game, arg(0));
  case SpotService::UNIT_SPOTTED:   return opeu_unit_special(game, arg(0));
  case SpotService::UNIT_ELIGIBLE:  return opeu_unit_selection_eligible(game, arg(0));
  case SpotService::GRID_DISTANCE:  return opeu_grid_distance(byte(0), byte(1), byte(2), byte(3));
  case SpotService::GRID_STEP:      return opeu_grid_step(game.State().Bytes(arg(0), 1).data(), game.State().Bytes(arg(1), 1).data(), byte(2));
  case SpotService::CELL_SIDE:      return opeu_cell_side(game, byte(0), byte(1));
  case SpotService::FIND_UNIT:      return opeu_find_unit_at(game, byte(0), byte(1));
  case SpotService::PENDING_FINISH: opeu_pending_finish(game, byte(0)); return 0;
  default:                          port_unhandled("movespot", id, count);
  }
  return 0;
}

auto port_routine_opeu_move_mines_finish(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_move_mines_finish(game, svc_movespot);
  return 0;
}
