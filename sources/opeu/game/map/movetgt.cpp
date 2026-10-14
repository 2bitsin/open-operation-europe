// Checks of a movement order's target (a cell next to the current unit, a friendly objective, a unit the side can
// see) and the order progress counters, plus the movement routines' shared services.
#include "map/movetgt.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "core/recindex.hpp"
#include "core/textout.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/movecost.hpp"
#include "map/movemine.hpp"
#include "map/movespot.hpp"
#include "map/movesup.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "map/toward.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
// opeu_move_unit_target's DS reserve for the unit name and the report the services write.
constexpr uint16_t REPORT_RESERVE{ 0x74 };
constexpr uint16_t NAME_AT{ 0 };
constexpr uint16_t REPORT_AT{ 0x0c };
// The message numbers naming the side that lost sight.
constexpr uint16_t SIDE_1_LOST{ 53 };
constexpr uint16_t SIDE_0_LOST{ 54 };
// Order progress: 0..99 counts down toward 0 (done), 100..199 counts up toward 200 (done), 255 not started.
constexpr uint8_t PROGRESS_UP{ 100 };
constexpr uint8_t PROGRESS_DONE_UP{ 200 };
constexpr uint8_t PROGRESS_UNSET{ 255 };
} // namespace

// Whether (x, y) is the current unit's cell or one next to it.
uint16_t opeu_move_adjacent(Game& game, uint8_t x, uint8_t y) {
  auto const& data{ Data(game) };
  auto const& unit{ Record<Unit>(data, data.current_unit_ptr) };
  auto const  reaches{ [&](uint8_t direction) { return StepOnMap({ unit.x, unit.y }, direction) == GridCell{ x, y }; } };
  return std::ranges::any_of(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS_AND_CENTER), reaches) ? 1 : 0;
}

// The objective at the current unit's order destination when the side holds it; otherwise the order is restored and
// no target returned.
auto opeu_move_objective_target(Game& game, uint8_t side) -> OrderTarget {
  auto const current{ Data(game).current_unit_ptr };
  if (auto const destination{ UnitOrderDestination(game, current) }; destination.valid) {
    auto const cell{ *destination.cell };
    auto const objective{ opeu_find_objective_at(game, cell.x, cell.y) };
    if (static_cast<uint8_t>(opeu_objective_side(game, objective)) == side) return { objective, cell };
  }
  opeu_restore_unit_order(game, current);
  return { 0, {} };
}

// The unit at the current unit's order destination. A player's side (or a computer side below delivery level 2) must
// still see it: a friendly unit must be selectable, an enemy one spotted; otherwise the loss of sight is reported, the
// order restored and 0 returned.
auto opeu_move_unit_target(Game& game) -> OrderTarget {
  auto const& data{ Data(game) };
  auto const  current{ data.current_unit_ptr };
  if (auto const destination{ UnitOrderDestination(game, current) }; destination.valid) {
    auto const cell{ *destination.cell };
    auto const target{ opeu_find_unit_at(game, cell.x, cell.y) };
    if (data.side_control[data.current_side] != SIDE_PLAYED && data.delivery_level >= 2) return { target, cell };
    auto const side{ static_cast<uint8_t>(opeu_unit_side(target)) };
    auto const seen{ side == data.current_side ? opeu_unit_selection_eligible(game, target) : opeu_unit_special(game, target) };
    if (seen) return { target, cell };
    TextBuffer<REPORT_RESERVE> locals;
    auto const                 report{ locals.At(REPORT_AT) };
    opeu_request_format(
        game, report,
        std::array<FormatArgument, 4>{ Near(data, data.lost_sight_format), side == 0 ? SIDE_0_LOST : SIDE_1_LOST, data.side_names[side], opeu_unit_name(game, target, locals.At(NAME_AT)) });
    opeu_unit_report(game, static_cast<std::uint16_t>(current), report);
  }
  opeu_restore_unit_order(game, current);
  return { 0, {} };
}

// Advances an upward order progress (an unset one starts at 200 - value); returns 1 when it reaches 200.
uint16_t opeu_move_level_raise(Game& game, uint16_t record, uint8_t value) {
  auto& progress{ Record<Order>(Data(game), record).progress };
  if (progress >= PROGRESS_UP && progress < PROGRESS_DONE_UP) ++progress;
  if (progress == PROGRESS_UNSET) progress = static_cast<uint8_t>(PROGRESS_DONE_UP - value);
  return progress == PROGRESS_DONE_UP;
}

// Advances a downward order progress (a finished or unset one starts at value); returns 1 when it reaches 0.
uint16_t opeu_move_level_lower(Game& game, uint16_t record, uint8_t value) {
  auto& progress{ Record<Order>(Data(game), record).progress };
  if (progress && progress < PROGRESS_UP) --progress;
  if (progress == PROGRESS_DONE_UP || progress == PROGRESS_UNSET) progress = value;
  return progress == 0;
}
