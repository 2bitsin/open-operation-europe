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
constexpr uint8_t NONE{ 255 };
constexpr uint8_t MINES_REQUEST{ 3 }; // the pending request the minefield report finishes
// The DS reserve of the step coordinates the grid step service moves.
constexpr uint16_t STEP_RESERVE{ 4 };
constexpr uint16_t STEP_X_AT{ 2 };
constexpr uint16_t STEP_Y_AT{ 0 };
} // namespace

void opeu_move_spot(Game& game) {
  auto&      data{ Data(game) };
  auto const current{ data.current_unit_ptr };
  auto&      mover{ Record<Unit>(data, current) };
  auto const x{ mover.x };
  auto const y{ mover.y };
  auto const enemy{ static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(data.current_side))) };
  auto const sight{ static_cast<uint8_t>(data.sight_ranges[data.weather] / (opeu_hour_is_night(static_cast<uint8_t>(data.hour)) ? 2 : 1)) };
  uint8_t    nearest{ NONE };
  uint16_t   spotter{ 0 };
  for (uint8_t i{ 0 }; i < UNITS_PER_SIDE; i++) {
    auto& other{ data.units[enemy * UNITS_PER_SIDE + i] };
    if (!(other.flags & Bits(UnitFlag::READY))) continue;
    auto const unit{ Near(data, other) };
    auto const distance{ static_cast<uint8_t>(opeu_grid_distance(static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(other.x), static_cast<uint8_t>(other.y))) };
    if (opeu_unit_selection_eligible(game, static_cast<std::uint16_t>(current)) && sight > distance) {
      auto const was_spotted{ opeu_unit_special(game, static_cast<std::uint16_t>(unit)) };
      for (auto& element : Elements(data, other))
        if (port_rand_bounded(game, static_cast<std::uint16_t>(sight)) >= distance) element.flags |= Bits(ElementFlag::SPOTTED);
      if (opeu_unit_special(game, static_cast<std::uint16_t>(unit)) != was_spotted && data.side_status[data.current_side] == SIDE_PLAYED) opeu_move_sighted(game, current, unit);
    }
    if (opeu_unit_selection_eligible(game, static_cast<std::uint16_t>(unit)) && distance < nearest) {
      nearest = distance;
      spotter = unit;
    }
  }
  if (nearest == NONE || (mover.flags & Bits(UnitFlag::SPOTTED))) return;
  auto const was_spotted{ opeu_unit_special(game, static_cast<std::uint16_t>(current)) };
  for (auto& element : Elements(data, mover)) {
    if (port_rand_bounded(game, static_cast<std::uint16_t>(sight)) >= nearest)
      element.flags |= Bits(ElementFlag::SPOTTED);
    else if (!port_rand_bounded(game, static_cast<std::uint16_t>(sight * 10)))
      element.flags &= ~Bits(ElementFlag::SPOTTED);
  }
  if (!was_spotted && opeu_unit_special(game, static_cast<std::uint16_t>(current)) != was_spotted && data.side_status[enemy] == SIDE_PLAYED) opeu_move_sighted(game, spotter, current);
}

uint16_t opeu_move_enemy_adjacent(Game& game) {
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
    if (!opeu_grid_step(game.State().Bytes(static_cast<std::uint16_t>(x_at), 1).data(), game.State().Bytes(static_cast<std::uint16_t>(y_at), 1).data(), static_cast<uint8_t>(direction))) continue;
    auto const side{ static_cast<std::uint16_t>(opeu_cell_side(game, static_cast<uint8_t>(Byte(game, x_at)), static_cast<uint8_t>(Byte(game, y_at)))) };
    if (static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(data.current_side))) != static_cast<uint8_t>(side)) continue;
    if (opeu_unit_special(game, static_cast<std::uint16_t>(opeu_find_unit_at(game, static_cast<uint8_t>(Byte(game, x_at)), static_cast<uint8_t>(Byte(game, y_at)))))) return 1;
  }
  return 0;
}

void opeu_move_mines_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(MINES_REQUEST));
}
