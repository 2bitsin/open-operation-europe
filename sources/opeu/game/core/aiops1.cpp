// The computer side's air planning: an air strike next to a contested objective or unit, and an airborne drop near an
// exposed enemy objective, with the cells and element types they use.
#include "core/aiops1.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/map-grid.hpp"
#include "core/aibudget.hpp"
#include "core/daylist.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
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

using namespace opeu::game;

uint16_t (*opeu_ai_unrecovered)(uint8_t* ds, uint16_t id, uint16_t ax, uint16_t dx, uint16_t* bx);

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

// The strike's target: the first of the eight cells around (x, y) that `accept` takes; false when none does.
template <typename Accept>
auto StepAround(std::uint8_t x, std::uint8_t y, std::uint8_t& to_x, std::uint8_t& to_y, Accept&& accept) -> bool {
  for (std::uint8_t direction{ 0 }; direction < GRID_DIRECTIONS; ++direction) {
    to_x = x;
    to_y = y;
    if (opeu_grid_step(&to_x, &to_y, direction) && accept(to_x, to_y)) return true;
  }
  return false;
}
} // namespace

// The side's transport planes not committed to the airlift or a pending drop, floored at 0.
uint16_t opeu_ai_spare_cargo(uint8_t ds[65536]) {
  auto&         side{ CurrentSide(Data(ds)) };
  std::uint16_t spare{ side.planes };
  opeu_sub_word(static_cast<std::uint16_t>(side.lift.planes + side.drop.transports), &spare);
  return spare;
}

// One time in six, plans an air strike: next to a held objective where an enemy special unit stands, at an enemy
// objective next to one of the side's units that can act, or next to a ready unit of the side where an enemy special
// unit stands. Returns 1 when a strike was planned.
uint16_t opeu_ai_002a(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  if (opeu_rand_bounded(6)) return 0;
  auto const side_index{ data.current_side };
  auto&      side{ data.sides[side_index] };
  if (!side.bombers) return 0;
  std::uint8_t x{};
  std::uint8_t y{};
  auto const   enemy_special{ [&](std::uint8_t cx, std::uint8_t cy) { return opeu_cell_side(ds, cx, cy) == opeu_opposite_side(side_index) && opeu_unit_special(ds, opeu_find_unit_at(ds, cx, cy)); } };
  bool         found{ false };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE && !found; ++i) {
    auto const near{ data.objective_links[side_index * OBJECTIVE_LINKS_PER_SIDE + i] };
    if (!near) continue;
    auto const& objective{ Record<Objective>(data, near) };
    if (!objective.active) continue;
    if (opeu_objective_side(ds, near) == side_index) {
      found = StepAround(objective.x, objective.y, x, y, [&](std::uint8_t cx, std::uint8_t cy) { return enemy_special(cx, cy) && !opeu_rand_bounded(2); });
    } else if (StepAround(objective.x, objective.y, x, y, [&](std::uint8_t cx, std::uint8_t cy) {
                 return opeu_cell_side(ds, cx, cy) == side_index && opeu_unit_selection_eligible(ds, opeu_find_unit_at(ds, cx, cy)) && !opeu_rand_bounded(4);
               })) {
      x     = objective.x;
      y     = objective.y;
      found = true;
    }
  }
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE && !found; ++i) {
    auto const& unit{ data.units[side_index * UNITS_PER_SIDE + i] };
    if (unit.flags & Bits(UnitFlag::READY)) found = StepAround(unit.x, unit.y, x, y, [&](std::uint8_t cx, std::uint8_t cy) { return enemy_special(cx, cy) && !opeu_rand_bounded(8); });
  }
  if (!found) return 0;
  auto& strike{ side.strike };
  strike.x      = x;
  strike.y      = y;
  strike.planes = opeu_min16(side.bombers, static_cast<std::uint16_t>(opeu_rand_bounded(11) + 10));
  strike.hours  = 1;
  strike.turns  = 7;
  return 1;
}

// The first airborne element type the side's nations field, or 255.
uint8_t opeu_ai_first_allowed_type(uint8_t ds[65536]) {
  auto const& data{ Data(ds) };
  auto const  nations{ data.side_masks[data.current_side] };
  for (std::uint8_t i{ 0 }; i < data.element_types.size(); ++i) {
    auto const& type{ data.element_types[i] };
    if ((type.flags & Bits(TypeFlag::AIRBORNE)) && (type.sides & nations)) return i;
  }
  return NO_TYPE;
}

// The first of the side's linked objectives held by the enemy whose cell is not under the side's influence, or 0.
uint16_t opeu_ai_exposed_enemy_objective(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE; ++i) {
    auto const near{ data.objective_links[data.current_side * OBJECTIVE_LINKS_PER_SIDE + i] };
    if (!near || opeu_objective_side(ds, near) == data.current_side) continue;
    auto const& objective{ Record<Objective>(data, near) };
    if (opeu_cell_influence_side(ds, objective.x, objective.y) != data.current_side) return near;
  }
  return 0;
}

// Tries up to five random cells within 3 of the objective; accepts an unmarked, unowned cell of drop terrain.
uint16_t opeu_ai_free_cell_near(uint8_t ds[65536], uint16_t objective, uint8_t* x, uint8_t* y) {
  auto& data{ Data(ds) };
  for (std::uint8_t attempt{ 0 }; attempt < 5; ++attempt) {
    auto const& target{ Record<Objective>(data, objective) };
    *x = static_cast<std::uint8_t>(opeu_rand_bounded(7) + target.x);
    *y = static_cast<std::uint8_t>(opeu_rand_bounded(7) + target.y);
    opeu_sub_byte(3, x);
    opeu_sub_byte(3, y);
    if (*x < MAP_WIDTH && *y < MAP_HEIGHT && IsDropTerrain(static_cast<std::uint8_t>(opeu_map_code(ds, *x, *y))) && !opeu_cell_marked(ds, *x, *y, data.current_side)
        && opeu_cell_side(ds, *x, *y) == NO_SIDE)
      return 1;
  }
  return 0;
}

// Unless the influence map mostly favours the side (and one time in twelve regardless), plans an airborne drop: a
// random free unit is filled with free elements of the first airborne type, up to the side's paratroops and spare
// transports, and the drop is aimed at a free cell near an exposed enemy objective. Returns 1 when a drop was planned.
uint16_t opeu_ai_03e2(uint8_t ds[65536]) {
  auto&        data{ Data(ds) };
  std::uint8_t favourable{ 0 };
  for (auto const influence : data.influence_map)
    if (data.current_side ? influence < 0 : influence > 0) ++favourable;
  if (opeu_rand_bounded(100) < favourable || !opeu_rand_bounded(12)) return 0;
  auto& drop{ CurrentSide(data).drop };
  drop.transports = 0;
  auto const paratroops{ CurrentSide(data).paratroops };
  if (!paratroops) return 0;
  auto const type{ opeu_ai_first_allowed_type(ds) };
  if (type == NO_TYPE) return 0;
  auto const planes{ opeu_ai_spare_cargo(ds) };
  if (!planes) return 0;
  auto const target{ opeu_ai_exposed_enemy_objective(ds) };
  if (!target || !opeu_ai_free_cell_near(ds, target, &drop.x, &drop.y)) return 0;
  auto const units{ AiFreeUnits(ds, UNIT_LIST, Bits(UnitFlag::AIRBORNE)) };
  if (!units) return 0;
  auto const elements{ AiFreeElements(ds, ELEMENT_LIST, Bits(ElementFlag::AIRBORNE)) };
  if (!elements) return 0;
  auto const    unit{ data.ai_unit_list[opeu_rand_bounded(units)] };
  auto const    slot{ opeu_unit_index(unit) };
  std::uint16_t troops_left{ paratroops };
  std::uint16_t planes_left{ planes };
  for (std::uint8_t i{ 0 }; !i || (troops_left && planes_left && data.ai_unit_element_counts[slot] > i && i < elements && opeu_rand_bounded(4)); ++i) {
    auto& element{ Record<Element>(data, data.ai_element_list[i]) };
    element.flags     = Bits(ElementFlag::AIRBORNE);
    element.type      = type;
    element.quantity  = opeu_min16(static_cast<std::uint16_t>(opeu_min16(planes_left, MAX_PLANES) * TROOPS_PER_PLANE), CurrentSide(data).paratroops);
    troops_left      -= element.quantity;
    planes_left      -= element.quantity / TROOPS_PER_PLANE;
    element.damage    = 0;
  }
  Record<Element>(data, data.ai_element_list[0]).flags |= Bits(ElementFlag::SELECTED);
  Record<Unit>(data, unit).flags                        = Bits(UnitFlag::AIRBORNE);
  drop.paratroops                                       = static_cast<std::uint16_t>(paratroops - troops_left);
  drop.transports                                       = static_cast<std::uint16_t>(planes - planes_left);
  drop.hours                                            = static_cast<std::uint8_t>(opeu_rand_bounded(5) + 1);
  drop.due                                              = static_cast<std::uint8_t>(drop.hours + 6);
  return 1;
}

uint16_t svc_ai_unrecovered(uint8_t* ds, uint16_t id, uint16_t ax, uint16_t dx, uint16_t* bx) {
  switch (static_cast<AiService>(id)) {
  case AiService::RELEASE_UNITS:      return opeu_release_side_units(ds, *bx, static_cast<uint8_t>(ax));
  case AiService::RELEASE_ELEMENTS:   return opeu_release_side_elements(ds, *bx, static_cast<uint8_t>(ax));
  case AiService::DUE_RECORDS:        return opeu_list_due_records(ds, *bx, static_cast<uint8_t>(ax), static_cast<uint8_t>(dx));
  case AiService::OBJECTIVE_LEVEL:    return opeu_objective_level(ds, *bx);
  case AiService::OBJECTIVE_ELIGIBLE: return opeu_objective_eligible(ds, *bx);
  case AiService::SUPPLY_SHARE:       return opeu_ai_supply_share(ds, bx, ax, static_cast<uint8_t>(dx));
  }
  port_unhandled("ai_unrecovered", id, 3);
  return 0;
}
