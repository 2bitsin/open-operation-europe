// A battle element's engagement and movement steps: the out-of-supply notice, the range checks, turning to face a
// cell, stepping into a cell and choosing the next step of a path.
#include "map/btlengag.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btldir.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlpath.hpp"
#include "map/btlunit.hpp"
#include "map/distance.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The supply notice's DS reserve: the unit name buffer and the message buffer, which the services fill.
constexpr uint16_t NOTICE_RESERVE{ 0x88 };
constexpr uint16_t NOTICE_NAME{ 0 };
constexpr uint16_t NOTICE_TEXT{ 0x20 };
// The DS reserves of the cell bytes the step and path services write, and their offsets in them.
constexpr uint16_t NEXT_RESERVE{ 0x10 };
constexpr uint16_t NEXT_X_AT{ 6 };
constexpr uint16_t NEXT_Y_AT{ 0 };
constexpr uint16_t PATH_RESERVE{ 8 };
constexpr uint16_t PATH_X_AT{ 2 };
constexpr uint16_t PATH_Y_AT{ 0 };
constexpr uint8_t  SUPPLY_FUEL{ 1 };
constexpr uint16_t COMMANDER_REPORT{ 2 };
constexpr uint8_t  NO_FACING{ 0xff };
constexpr uint8_t  STEP_DIRECTIONS{ 4 };
constexpr uint16_t PATH_TO_CELL{ 1 };
constexpr uint16_t PATH_TO_GOAL{ 3 };

// What opeu_battle_step_into reports.
enum class StepResult : uint8_t {
  BLOCKED = 1,
  ARRIVED = 2,
  MOVED   = 3
};
// What opeu_battle_next_step reports.
enum class NextStep : uint8_t {
  WAIT    = 1,
  NO_PATH = 2,
  STUCK   = 3,
  FOUND   = 4
};

void Pause(Game& game) {
  opeu_battle_pause(game, static_cast<uint8_t>(2));
}

[[nodiscard]] auto FindPath(Game& game, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint16_t mode) -> uint16_t {
  return opeu_battle_path_flood(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<uint8_t>(to_x), static_cast<uint8_t>(to_y), element, static_cast<uint8_t>(mode));
}
} // namespace

// "<unit> is out of <supply>! They cannot move/fight!", for a side a player commands.
void opeu_battle_supply_notice(Game& game, uint16_t element, uint8_t supply) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NOTICE_RESERVE };
  auto const          text{ locals.At(NOTICE_TEXT) };
  auto const          name_buffer{ locals.At(NOTICE_NAME) };
  auto const          side{ opeu_battle_element_side(game, element) };
  if (!data.side_status[side]) return;
  auto const unit{ opeu_battle_element_unit(game, element) };
  auto const name{ unit == Near(data, data.battle_garrison) ? Near(data, data.text_defenders) : opeu_unit_name(game, unit, name_buffer) };
  opeu_request_format(game, std::array<std::uint16_t, 6>{ static_cast<std::uint16_t>(text), static_cast<std::uint16_t>(Near(data, data.supply_notice_format)),
                                                          static_cast<std::uint16_t>(SideColorCode(side)), static_cast<std::uint16_t>(name), static_cast<std::uint16_t>(data.supply_names[supply]),
                                                          static_cast<std::uint16_t>(supply == SUPPLY_FUEL ? Near(data, data.text_cannot_move) : Near(data, data.text_cannot_fight)) });
  opeu_battle_commander_wait(game, text, static_cast<uint8_t>(side), static_cast<uint8_t>(COMMANDER_REPORT));
}

// Whether the element's target is still in sight and within its type's reach over the terrain.
uint16_t opeu_battle_target_in_range(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  auto const target{ PositionOf(data, element).target };
  if (!opeu_battle_element_active(game, target) || !opeu_battle_element_flag2(game, target)) {
    opeu_battle_update_element(game, element);
    return 0;
  }
  return ElementTypeOf(data, element).gun_range < opeu_battle_terrain_mode(game, element, target) ? 0 : 1;
}

// Whether the element's goal cell is within its type's reach.
uint16_t opeu_battle_cell_in_range(Game& game, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& position{ PositionOf(data, element) };
  return ElementTypeOf(data, element).gun_range < opeu_grid_distance(position.x, position.y, position.goal_x, position.goal_y) ? 0 : 1;
}

// Infantry flash once; other elements turn through the in-between facing to the new one.
void opeu_battle_animate_facing(Game& game, uint16_t element, uint8_t mode) {
  auto&      data{ Data(game) };
  auto const x{ PositionOf(data, element).x };
  auto const y{ PositionOf(data, element).y };
  auto const current{ opeu_battle_mode(game, element) };
  auto const draw{ [&](uint8_t facing) {
    opeu_battle_redraw_cell(game, x, y, element, facing);
    Pause(game);
  } };
  auto const facing{ [](CellFacing f) { return std::to_underlying(f); } };
  if (IsOfCategory(ElementTypeOf(data, element), Category::INFANTRY)) {
    draw(facing(CellFacing::DOWN));
    draw(facing(CellFacing::UP));
    return;
  }
  if (current == mode) return;
  auto const up{ facing(CellFacing::UP) };
  auto const down{ facing(CellFacing::DOWN) };
  auto const left{ facing(CellFacing::LEFT) };
  auto const right{ facing(CellFacing::RIGHT) };
  if ((current == up && mode == down) || (current == down && mode == up)) draw(right);
  if ((current == left && mode == right) || (current == right && mode == left)) draw(up);
  draw(mode);
}

// An element short of fuel cannot move; into a free cell it turns and moves, otherwise it stops facing nowhere and
// reports whether it stands at its goal (its target's cell when engaged).
uint8_t opeu_battle_step_into(Game& game, uint16_t element, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  if (ElementTypeOf(data, element).fuel_capacity && Record<Unit>(data, unit).resources[1] == 0) {
    opeu_battle_supply_notice(game, element, SUPPLY_FUEL);
    return std::to_underlying(StepResult::ARRIVED);
  }
  if (!opeu_battle_cell_occupied(game, x, y)) {
    uint8_t const facing{ IsOfCategory(ElementTypeOf(data, element), Category::INFANTRY) ? uint8_t{ 0 }
                                                                                         : opeu_battle_cell_facing(game, PositionOf(data, element).x, PositionOf(data, element).y, x, y) };
    opeu_battle_animate_facing(game, element, facing);
    auto const from_x{ PositionOf(data, element).x };
    auto const from_y{ PositionOf(data, element).y };
    opeu_battle_move_element(game, from_x, from_y, x, y, element, facing);
    opeu_battle_spot_enemies(game, element);
    opeu_battle_set_order(game, element, 0);
    return std::to_underlying(StepResult::MOVED);
  }
  opeu_battle_set_facing(game, element, NO_FACING);
  uint8_t goal_x;
  uint8_t goal_y;
  if (opeu_battle_engaged(game, element)) {
    auto const& target{ PositionOf(data, PositionOf(data, element).target) };
    goal_x = target.x;
    goal_y = target.y;
  } else {
    goal_x = PositionOf(data, element).goal_x;
    goal_y = PositionOf(data, element).goal_y;
  }
  if (goal_x == x && goal_y == y) return std::to_underlying(StepResult::ARRIVED);
  return std::to_underlying(StepResult::BLOCKED);
}

// x, y: DS offsets of the destination cell bytes; on a path found they receive its next step. The element waits
// while its order counter is below the cheapest free neighbour's cost.
uint8_t opeu_battle_next_step(Game& game, uint16_t element, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NEXT_RESERVE };
  auto const          next_x{ locals.At(NEXT_X_AT) };
  auto const          next_y{ locals.At(NEXT_Y_AT) };
  auto const          from_here{ [&] {
    Byte(game, next_x) = PositionOf(data, element).x;
    Byte(game, next_y) = PositionOf(data, element).y;
  } };
  uint8_t cheapest{ IMPASSABLE_COST };
  for (uint8_t direction{ 0 }; direction < STEP_DIRECTIONS; ++direction) {
    from_here();
    if (opeu_battle_step(game, game.State().Bytes(next_x, 1).data(), game.State().Bytes(next_y, 1).data(), static_cast<uint8_t>(direction))) {
      auto const cost{ static_cast<uint8_t>(opeu_battle_move_cost(game, static_cast<uint8_t>(Byte(game, next_x)), static_cast<uint8_t>(Byte(game, next_y)), element)) };
      if (cost != IMPASSABLE_COST && !opeu_battle_cell_occupied(game, Byte(game, next_x), Byte(game, next_y)) && cost < cheapest) cheapest = cost;
    }
  }
  if (cheapest == IMPASSABLE_COST) return std::to_underlying(NextStep::STUCK);
  if (opeu_battle_order(game, element) < cheapest) return std::to_underlying(NextStep::WAIT);
  from_here();
  auto const to_x{ Byte(game, x) };
  auto const to_y{ Byte(game, y) };
  if (!FindPath(game, to_x, to_y, Byte(game, next_x), Byte(game, next_y), element, PATH_TO_CELL)) return std::to_underlying(NextStep::NO_PATH);
  opeu_battle_path_step(game, next_x, next_y);
  Byte(game, x) = Byte(game, next_x);
  Byte(game, y) = Byte(game, next_y);
  return std::to_underlying(NextStep::FOUND);
}

// x, y: DS offsets of the goal cell bytes; on a path found they receive its next step.
uint16_t opeu_battle_element_path_step(Game& game, uint16_t element, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ PATH_RESERVE };
  auto const          step_x{ locals.At(PATH_X_AT) };
  auto const          step_y{ locals.At(PATH_Y_AT) };
  Byte(game, step_x) = PositionOf(data, element).x;
  Byte(game, step_y) = PositionOf(data, element).y;
  auto const to_x{ Byte(game, x) };
  auto const to_y{ Byte(game, y) };
  if (!FindPath(game, to_x, to_y, Byte(game, step_x), Byte(game, step_y), element, PATH_TO_GOAL)) return 0;
  opeu_battle_path_step(game, step_x, step_y);
  Byte(game, x) = Byte(game, step_x);
  Byte(game, y) = Byte(game, step_y);
  return 1;
}
