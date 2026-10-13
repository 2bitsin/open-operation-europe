// The computer side's passes after its turn: staff rotation, division leaders, restocking units from objectives,
// supply and equipment requests, raising a new regiment, air plans and the attack target.
#include "core/aiops5.hpp"
#include "core/grid-neighbours.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/aiops1.hpp"
#include "core/aiops2.hpp"
#include "core/aiops3.hpp"
#include "core/aiops4.hpp"
#include "core/limits.hpp"
#include "core/random.hpp"
#include "core/recindex.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "units/value.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  NO_REQUEST{ 0xff };
constexpr std::uint8_t  RAISE_REQUEST{ 2 };   // the request slot and configuration kind of a new regiment
constexpr std::uint8_t  REQUEST_FILED{ 2 };   // RequestSlot::kind of the filed regiment request
constexpr std::uint8_t  REINFORCE_DELAY{ 6 }; // reinforcements wait while the side's request is this far along
constexpr std::uint16_t UNIT_LIST{ offsetof(DataSegment, ai_unit_list) };
constexpr std::uint16_t ELEMENT_LIST{ offsetof(DataSegment, ai_element_list) };

// The equipment kinds a new regiment is raised with: whether an element type of `category` suits the kind for
// `side`.
auto SuitsKind(std::uint8_t kind, std::uint8_t side, std::uint8_t category) -> bool {
  switch (kind) {
  case 0:  return category == 0;
  case 1:  return category == 1;
  case 2:  return (side == 0 && category == 3) || (side == 1 && category == 2);
  case 3:  return (side == 0 && category == 4) || (side == 1 && category == 0);
  case 4:  return category == 5;
  case 5:  return (side == 0 && category == 7) || (side == 1 && category == 6);
  default: return true;
  }
}
} // namespace

// Loads the element with a random quantity of a random element type suiting `kind` for `side`: types are drawn by
// the side's request weights among those the side's nations field.
void opeu_ai_equip_element(Game& game, uint16_t element, uint8_t kind, uint8_t side) {
  auto& data{ Data(game) };
  for (;;) {
    auto const index{ static_cast<std::uint8_t>(opeu_rand_bounded(ELEMENT_TYPE_COUNT)) };
    auto const weight{ data.type_availability[index] };
    if (!weight) continue;
    auto const& type{ data.element_types[index] };
    if (!SuitsKind(kind, side, type.category) || !(data.side_masks[data.current_side] & type.sides) || weight <= opeu_rand_bounded(100)) continue;
    auto&      load{ Record<Element>(data, element) };
    auto const capacity{ data.request_capacities[type.category] };
    auto const typical{ static_cast<std::uint16_t>(capacity / (opeu_max16(type.anti_tank, type.anti_personnel) / 20 + 1) + 18) };
    load.type     = index;
    load.quantity = opeu_min16(opeu_rand_bounded(typical / 3) + (typical >> 1), capacity);
    load.damage   = 0;
    return;
  }
}

// Unless a regiment request is pending or configured: raises a random free unit as a requested regiment of a random
// kind (the support kinds 4 and 5 for the regiment slots of those categories) and equips free elements for it, at
// least two, at most the slot's element count, each with morale and training by the delivery level. 1 when raised.
uint16_t opeu_ai_raise_unit(Game& game) {
  if (opeu_ai_request_pending(game, RAISE_REQUEST) || opeu_ai_config_kind_is(game, RAISE_REQUEST)) return 0;
  auto const units{ AiFreeUnits(game, UNIT_LIST, Bits(UnitFlag::REQUESTED)) };
  if (!units) return 0;
  auto elements{ AiFreeElements(game, ELEMENT_LIST, Bits(ElementFlag::REQUESTED)) };
  if (!elements) return 0;
  auto&        data{ Data(game) };
  auto const   unit{ data.ai_unit_list[opeu_rand_bounded(units)] };
  auto const   slot{ opeu_unit_index(unit) };
  std::uint8_t kind{};
  if (opeu_category_6(slot))
    kind = 4;
  else if (opeu_category_7(slot))
    kind = 5;
  else
    kind = static_cast<std::uint8_t>(opeu_rand_bounded(4));
  Record<Unit>(data, unit).flags = Bits(UnitFlag::REQUESTED);
  for (std::uint8_t i{ 0 }; i < data.ai_unit_element_counts[slot]; ++i) {
    if (!opeu_rand_bounded(data.ai_unit_element_counts[slot] + data.delivery_level) && i > 1) break;
    if (!elements) break;
    auto const near{ data.ai_element_list[i] };
    opeu_ai_equip_element(game, near, kind, opeu_rand_bounded(2));
    auto& element{ Record<Element>(data, near) };
    element.morale     = static_cast<std::uint8_t>(opeu_rand_bounded(20) + 20 * data.delivery_level);
    element.training   = static_cast<std::uint8_t>(opeu_rand_bounded(20) + 20 * data.delivery_level);
    element.experience = static_cast<std::uint8_t>(opeu_rand_bounded(10));
    element.flags      = Bits(ElementFlag::REQUESTED);
    --elements;
  }
  auto& request{ Record<Side>(data, data.current_side_ptr).requests[RAISE_REQUEST] };
  request.target   = 0;
  request.argument = kind;
  request.kind     = REQUEST_FILED;
  return 1;
}

// While the side's request configuration is idle or not yet far along: files supply and equipment requests and
// raises a regiment.
void opeu_ai_reinforce(Game& game) {
  auto const& config{ Data(game).request_configs[Data(game).current_side] };
  if (config.kind == NO_REQUEST || config.delay < REINFORCE_DELAY) {
    opeu_ai_request_resources(game);
    opeu_ai_request_equipment(game);
    opeu_ai_raise_unit(game);
  }
}

// Moves the objective's stock of each of the unit's element types into the elements, up to the type's capacity.
// 1 when anything moved.
uint16_t opeu_ai_restock_unit(Game& game, uint16_t objective, uint16_t unit) {
  auto&         data{ Data(game) };
  auto&         store{ Record<Objective>(data, objective) };
  std::uint16_t moved{ 0 };
  for (auto& element : Elements(data, Record<Unit>(data, unit).first)) {
    for (auto& item : store.items) {
      if (!item.quantity || item.type != element.type) continue;
      auto const amount{ opeu_min16(data.request_capacities[data.element_types[element.type].category] - element.quantity, item.quantity) };
      element.quantity += amount;
      item.quantity    -= amount;
      moved             = 1;
    }
  }
  return moved;
}

// Restocks the side's units on and around each of its objectives that may be planned for. 1 when anything moved.
uint16_t opeu_ai_restock_from_objectives(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t moved{ 0 };
  for (auto& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (!objective.active || opeu_objective_side(game, near) != data.current_side || !AiObjectiveEligible(game, near)) continue;
    for (auto const [x, y] : CellsAround({ objective.x, objective.y }, GRID_DIRECTIONS_AND_CENTER))
      if (opeu_cell_side(game, x, y) == data.current_side && opeu_ai_restock_unit(game, near, opeu_find_unit_at(game, x, y))) moved = 1;
  }
  return moved;
}

// In each of the side's ready divisions, selects the first ready unit and deselects the others.
void opeu_ai_mark_group_leaders(Game& game) {
  auto& data{ Data(game) };
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
    if (!(GroupOf(data, data.current_side, group).flags & Bits(GroupFlag::READY))) continue;
    bool marked{ false };
    for (auto& unit : GroupUnits(data, data.current_side, group)) {
      if (!marked && (unit.flags & Bits(UnitFlag::READY))) {
        unit.flags |= Bits(UnitFlag::SELECTED);
        marked      = true;
      } else
        unit.flags &= static_cast<std::uint8_t>(~Bits(UnitFlag::SELECTED));
    }
  }
}

// Marks every ready division of the side as handled by the computer.
void opeu_ai_15c0(Game& game) {
  auto& data{ Data(game) };
  for (unsigned group{ 0 }; group < GROUPS_PER_SIDE; ++group) {
    auto& division{ GroupOf(data, data.current_side, group) };
    if (division.flags & Bits(GroupFlag::READY)) division.flags |= Bits(GroupFlag::AI_MARK);
  }
}

// The computer side's passes after the other side's turn control.
void opeu_control_other_finish(Game& game) {
  opeu_ai_15c0(game);
  opeu_ai_rotate_staff(game);
  opeu_ai_mark_group_leaders(game);
  opeu_ai_refresh_capacities(game);
  opeu_ai_restock_from_objectives(game);
  opeu_ai_reinforce(game);
  opeu_ai_daytime_plans(game);
  opeu_ai_choose_target(game);
}
