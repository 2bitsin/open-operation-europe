// The computer side's air planning: an air strike next to a contested objective or unit, and an airborne drop near an
// exposed enemy objective, with the cells and element types they use.
#include "core/aiops1.hpp"
#include "core/grid-neighbours.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "core/aibudget.hpp"
#include "core/daylist.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/objeleg.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "units/release.hpp"
#include "units/stats.hpp"

#include <cstddef>
#include <optional>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  NO_TYPE{ 0xff };
constexpr std::uint16_t TROOPS_PER_PLANE{ 30 };
constexpr std::uint16_t MAX_PLANES{ 33 }; // the most transports one drop takes
constexpr std::uint16_t UNIT_LIST{ offsetof(DataSegment, ai_unit_list) };
constexpr std::uint16_t ELEMENT_LIST{ offsetof(DataSegment, ai_element_list) };

// Terrain codes where no airborne drop lands.
constexpr auto IsDropTerrain(std::uint8_t code) -> bool {
  switch (code) {
  case 0:
  case 1:
  case 5:
  case 6:
  case 8:
  case 10: return false;
  default: return true;
  }
}

// The last cell a search tried, and whether it was what the search looked for.
struct CellSearch {
  GridCell cell;
  bool     found;
};

// Whether airborne troops can land on the cell: on the map, of drop terrain, not marked by the side and held by no side.
auto FreeForDrop(Game& game, GridCell cell) -> bool {
  return cell.x < MAP_WIDTH && cell.y < MAP_HEIGHT && IsDropTerrain(opeu_map_code(game, cell.x, cell.y)) && !opeu_cell_marked(game, cell.x, cell.y, Data(game).current_side)
         && opeu_cell_side(game, cell.x, cell.y) == NO_SIDE;
}

// A random cell within 3 of the objective (clamped at the top and left edges).
auto RandomCellNear(Game& game, Objective const& objective) -> GridCell {
  auto x{ static_cast<std::uint8_t>(game.State().Generator().Bounded(7) + objective.x) };
  auto y{ static_cast<std::uint8_t>(game.State().Generator().Bounded(7) + objective.y) };
  x = SubByte(x, 3);
  y = SubByte(y, 3);
  return { x, y };
}

// Tries up to five random cells within 3 of the objective until one is free for a drop.
auto FreeCellNear(Game& game, Objective const& objective) -> CellSearch {
  CellSearch search{};
  for ([[maybe_unused]] auto const attempt : std::views::iota(0, 5)) {
    search.cell = RandomCellNear(game, objective);
    if (FreeForDrop(game, search.cell)) return { search.cell, true };
  }
  return search;
}

// The first of the eight cells around `cell` that `accept` takes.
template <typename Accept>
auto StepAround(GridCell cell, Accept&& accept) -> std::optional<GridCell> {
  for (auto const around : CellsAround(cell))
    if (accept(around)) return around;
  return std::nullopt;
}

// Whether an enemy special unit of the side stands on the cell.
auto EnemySpecialAt(Game& game, std::uint8_t side, GridCell cell) -> bool {
  return opeu_cell_side(game, cell.x, cell.y) == opeu_opposite_side(side) && opeu_unit_special(game, opeu_find_unit_at(game, cell.x, cell.y));
}

// A strike target at the side's linked objectives: next to a held one, a cell where an enemy special unit stands (each
// such cell one time in two); an enemy one itself, when one of the side's units that can act stands next to it (each
// one time in four).
auto TargetAtObjectives(Game& game, std::uint8_t side) -> std::optional<GridCell> {
  auto& data{ Data(game) };
  for (auto const near : std::span{ data.objective_links }.subspan(side * OBJECTIVE_LINKS_PER_SIDE, OBJECTIVE_LINKS_PER_SIDE)) {
    if (!near) continue;
    auto const& objective{ Record<Objective>(data, near) };
    if (!objective.active) continue;
    GridCell const cell{ objective.x, objective.y };
    if (opeu_objective_side(game, near) == side) {
      if (auto const target{ StepAround(cell, [&](GridCell around) { return EnemySpecialAt(game, side, around) && !game.State().Generator().Bounded(2); }) }) return target;
    } else if (StepAround(cell, [&](GridCell around) {
                 return opeu_cell_side(game, around.x, around.y) == side && opeu_unit_selection_eligible(game, opeu_find_unit_at(game, around.x, around.y)) && !game.State().Generator().Bounded(4);
               }))
      return cell;
  }
  return std::nullopt;
}

// A strike target next to one of the side's ready units: a cell where an enemy special unit stands (each such cell
// one time in eight).
auto TargetAtUnits(Game& game, std::uint8_t side) -> std::optional<GridCell> {
  auto& data{ Data(game) };
  for (auto const& unit : std::span{ data.units }.subspan(side * UNITS_PER_SIDE, UNITS_PER_SIDE)) {
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    if (auto const target{ StepAround({ unit.x, unit.y }, [&](GridCell around) { return EnemySpecialAt(game, side, around) && !game.State().Generator().Bounded(8); }) }) return target;
  }
  return std::nullopt;
}
} // namespace

// The side's transport planes not committed to the airlift or a pending drop, floored at 0.
uint16_t opeu_ai_spare_cargo(Game& game) {
  auto&         side{ CurrentSide(Data(game)) };
  std::uint16_t spare{ side.planes };
  spare = SubWord(spare, side.lift.planes + side.drop.transports);
  return spare;
}

// One time in six, plans an air strike: next to a held objective where an enemy special unit stands, at an enemy
// objective next to one of the side's units that can act, or next to a ready unit of the side where an enemy special
// unit stands. Returns 1 when a strike was planned.
uint16_t opeu_ai_002a(Game& game) {
  auto& data{ Data(game) };
  if (game.State().Generator().Bounded(6)) return 0;
  auto const side_index{ data.current_side };
  auto&      side{ data.sides[side_index] };
  if (!side.bombers) return 0;
  auto target{ TargetAtObjectives(game, side_index) };
  if (!target) target = TargetAtUnits(game, side_index);
  if (!target) return 0;
  auto& strike{ side.strike };
  strike.x      = target->x;
  strike.y      = target->y;
  strike.planes = opeu_min16(side.bombers, game.State().Generator().Bounded(11) + 10);
  strike.hours  = 1;
  strike.turns  = 7;
  return 1;
}

// The first airborne element type the side's nations field, or 255.
uint8_t opeu_ai_first_allowed_type(Game& game) {
  auto const& data{ Data(game) };
  auto const  nations{ data.side_masks[data.current_side] };
  for (std::uint8_t i{ 0 }; i < data.element_types.size(); ++i) {
    auto const& type{ data.element_types[i] };
    if ((type.flags & Bits(TypeFlag::AIRBORNE)) && (type.sides & nations)) return i;
  }
  return NO_TYPE;
}

// The first of the side's linked objectives held by the enemy whose cell is not under the side's influence, or 0.
uint16_t opeu_ai_exposed_enemy_objective(Game& game) {
  auto& data{ Data(game) };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE; ++i) {
    auto const near{ data.objective_links[data.current_side * OBJECTIVE_LINKS_PER_SIDE + i] };
    if (!near || opeu_objective_side(game, near) == data.current_side) continue;
    auto const& objective{ Record<Objective>(data, near) };
    if (opeu_cell_influence_side(game, objective.x, objective.y) != data.current_side) return near;
  }
  return 0;
}

// Unless the influence map mostly favours the side (and one time in twelve regardless), plans an airborne drop: a
// random free unit is filled with free elements of the first airborne type, up to the side's paratroops and spare
// transports, and the drop is aimed at a free cell near an exposed enemy objective. Returns 1 when a drop was planned.
uint16_t opeu_ai_03e2(Game& game) {
  auto&        data{ Data(game) };
  std::uint8_t favourable{ 0 };
  for (auto const influence : data.influence_map)
    if (data.current_side ? influence < 0 : influence > 0) ++favourable;
  if (game.State().Generator().Bounded(100) < favourable || !game.State().Generator().Bounded(12)) return 0;
  auto& drop{ CurrentSide(data).drop };
  drop.transports = 0;
  auto const paratroops{ CurrentSide(data).paratroops };
  if (!paratroops) return 0;
  auto const type{ opeu_ai_first_allowed_type(game) };
  if (type == NO_TYPE) return 0;
  auto const planes{ opeu_ai_spare_cargo(game) };
  if (!planes) return 0;
  auto const target{ opeu_ai_exposed_enemy_objective(game) };
  if (!target) return 0;
  auto const drop_cell{ FreeCellNear(game, Record<Objective>(data, target)) };
  drop.x = drop_cell.cell.x;
  drop.y = drop_cell.cell.y;
  if (!drop_cell.found) return 0;
  auto const units{ AiFreeUnits(game, UNIT_LIST, Bits(UnitFlag::AIRBORNE)) };
  if (!units) return 0;
  auto const elements{ AiFreeElements(game, ELEMENT_LIST, Bits(ElementFlag::AIRBORNE)) };
  if (!elements) return 0;
  auto const    unit{ data.ai_unit_list[game.State().Generator().Bounded(units)] };
  auto const    slot{ opeu_unit_index(unit) };
  std::uint16_t troops_left{ paratroops };
  std::uint16_t planes_left{ planes };
  for (std::uint8_t i{ 0 }; !i || (troops_left && planes_left && data.ai_unit_element_counts[slot] > i && i < elements && game.State().Generator().Bounded(4)); ++i) {
    auto& element{ Record<Element>(data, data.ai_element_list[i]) };
    element.flags     = Bits(ElementFlag::AIRBORNE);
    element.type      = type;
    element.quantity  = opeu_min16(opeu_min16(planes_left, MAX_PLANES) * TROOPS_PER_PLANE, CurrentSide(data).paratroops);
    troops_left      -= element.quantity;
    planes_left      -= element.quantity / TROOPS_PER_PLANE;
    element.damage    = 0;
  }
  Record<Element>(data, data.ai_element_list[0]).flags |= Bits(ElementFlag::SELECTED);
  Record<Unit>(data, unit).flags                        = Bits(UnitFlag::AIRBORNE);
  drop.paratroops                                       = static_cast<std::uint16_t>(paratroops - troops_left);
  drop.transports                                       = static_cast<std::uint16_t>(planes - planes_left);
  drop.hours                                            = static_cast<std::uint8_t>(game.State().Generator().Bounded(5) + 1);
  drop.due                                              = static_cast<std::uint8_t>(drop.hours + 6);
  return 1;
}
