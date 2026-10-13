// A battle element's engagement and movement steps: the out-of-supply notice, the range checks, turning to face a
// cell, stepping into a cell and choosing the next step of a path.
#include "map/btlengag.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "map/btldir.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/btlunit.hpp"
#include "map/distance.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

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

void Pause(Game& game, BattleEvent event) {
  Call(game, event, BattleRoutine::BATTLE_PAUSE, 2);
}

[[nodiscard]] auto FindPath(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y, uint16_t element, uint16_t mode) -> uint16_t {
  return Call(game, event, BattleRoutine::BATTLE_PATH_FLOOD, x, y, to_x, to_y, element, mode);
}
} // namespace

// "<unit> is out of <supply>! They cannot move/fight!", for a side a player commands.
void opeu_battle_supply_notice(Game& game, BattleEvent event, uint16_t element, uint8_t supply) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NOTICE_RESERVE };
  auto const          text{ locals.At(NOTICE_TEXT) };
  auto const          name_buffer{ locals.At(NOTICE_NAME) };
  auto const          side{ opeu_battle_element_side(game, event, element) };
  if (!data.side_status[side]) return;
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  auto const name{ unit == Near(data, data.battle_garrison) ? Near(data, data.text_defenders) : Call(game, event, BattleRoutine::UNIT_NAME, name_buffer, unit) };
  Call(game, event, BattleRoutine::REQUEST_FORMAT, text, Near(data, data.supply_notice_format), SideColorCode(side), name, data.supply_names[supply],
       supply == SUPPLY_FUEL ? Near(data, data.text_cannot_move) : Near(data, data.text_cannot_fight));
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_WAIT, side, COMMANDER_REPORT, text);
}

// Whether the element's target is still in sight and within its type's reach over the terrain.
uint16_t opeu_battle_target_in_range(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  auto const target{ PositionOf(data, element).target };
  if (!opeu_battle_element_active(game, event, target) || !opeu_battle_element_flag2(game, event, target)) {
    opeu_battle_update_element(game, event, element);
    return 0;
  }
  return ElementTypeOf(data, element).gun_range < opeu_battle_terrain_mode(game, event, element, target) ? 0 : 1;
}

// Whether the element's goal cell is within its type's reach.
uint16_t opeu_battle_cell_in_range(Game& game, BattleEvent, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& position{ PositionOf(data, element) };
  return ElementTypeOf(data, element).gun_range < opeu_grid_distance(position.x, position.y, position.goal_x, position.goal_y) ? 0 : 1;
}

// Infantry flash once; other elements turn through the in-between facing to the new one.
void opeu_battle_animate_facing(Game& game, BattleEvent event, uint16_t element, uint8_t mode) {
  auto&      data{ Data(game) };
  auto const x{ PositionOf(data, element).x };
  auto const y{ PositionOf(data, element).y };
  auto const current{ opeu_battle_mode(game, event, element) };
  auto const draw{ [&](uint8_t facing) {
    opeu_battle_redraw_cell(game, event, x, y, element, facing);
    Pause(game, event);
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
uint8_t opeu_battle_step_into(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  if (ElementTypeOf(data, element).fuel_capacity && Record<Unit>(data, unit).resources[1] == 0) {
    opeu_battle_supply_notice(game, event, element, SUPPLY_FUEL);
    return std::to_underlying(StepResult::ARRIVED);
  }
  if (!opeu_battle_cell_occupied(game, event, x, y)) {
    uint8_t const facing{ IsOfCategory(ElementTypeOf(data, element), Category::INFANTRY) ? uint8_t{ 0 }
                                                                                         : opeu_battle_cell_facing(game, event, PositionOf(data, element).x, PositionOf(data, element).y, x, y) };
    opeu_battle_animate_facing(game, event, element, facing);
    auto const from_x{ PositionOf(data, element).x };
    auto const from_y{ PositionOf(data, element).y };
    opeu_battle_move_element(game, event, from_x, from_y, x, y, element, facing);
    Call(game, event, BattleRoutine::BATTLE_SPOT_ENEMIES, element);
    opeu_battle_set_order(game, event, element, 0);
    return std::to_underlying(StepResult::MOVED);
  }
  opeu_battle_set_facing(game, event, element, NO_FACING);
  uint8_t goal_x;
  uint8_t goal_y;
  if (opeu_battle_engaged(game, event, element)) {
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
uint8_t opeu_battle_next_step(Game& game, BattleEvent event, uint16_t element, uint16_t x, uint16_t y) {
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
    if (Call(game, event, BattleRoutine::BATTLE_STEP, next_y, direction, next_x)) {
      auto const cost{ static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_MOVE_COST, Byte(game, next_x), Byte(game, next_y), element)) };
      if (cost != IMPASSABLE_COST && !opeu_battle_cell_occupied(game, event, Byte(game, next_x), Byte(game, next_y)) && cost < cheapest) cheapest = cost;
    }
  }
  if (cheapest == IMPASSABLE_COST) return std::to_underlying(NextStep::STUCK);
  if (opeu_battle_order(game, event, element) < cheapest) return std::to_underlying(NextStep::WAIT);
  from_here();
  auto const to_x{ Byte(game, x) };
  auto const to_y{ Byte(game, y) };
  if (!FindPath(game, event, to_x, to_y, Byte(game, next_x), Byte(game, next_y), element, PATH_TO_CELL)) return std::to_underlying(NextStep::NO_PATH);
  Call(game, event, BattleRoutine::BATTLE_PATH_STEP, next_y, next_x);
  Byte(game, x) = Byte(game, next_x);
  Byte(game, y) = Byte(game, next_y);
  return std::to_underlying(NextStep::FOUND);
}

// x, y: DS offsets of the goal cell bytes; on a path found they receive its next step.
uint16_t opeu_battle_element_path_step(Game& game, BattleEvent event, uint16_t element, uint16_t x, uint16_t y) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ PATH_RESERVE };
  auto const          step_x{ locals.At(PATH_X_AT) };
  auto const          step_y{ locals.At(PATH_Y_AT) };
  Byte(game, step_x) = PositionOf(data, element).x;
  Byte(game, step_y) = PositionOf(data, element).y;
  auto const to_x{ Byte(game, x) };
  auto const to_y{ Byte(game, y) };
  if (!FindPath(game, event, to_x, to_y, Byte(game, step_x), Byte(game, step_y), element, PATH_TO_GOAL)) return 0;
  Call(game, event, BattleRoutine::BATTLE_PATH_STEP, step_y, step_x);
  Byte(game, x) = Byte(game, step_x);
  Byte(game, y) = Byte(game, step_y);
  return 1;
}

uint16_t svc_btlengag(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_animate_facing(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_animate_facing(game, svc_btlengag, Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
  return 0;
}

auto port_routine_opeu_battle_cell_in_range(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_cell_in_range(game, svc_btlengag, Arg(args, count, 0));
}

auto port_routine_opeu_battle_element_path_step(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_element_path_step(game, svc_btlengag, Arg(args, count, 2), Arg(args, count, 0), Arg(args, count, 1));
}

auto port_routine_opeu_battle_next_step(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_next_step(game, svc_btlengag, Arg(args, count, 2), Arg(args, count, 0), Arg(args, count, 1));
}

auto port_routine_opeu_battle_step_into(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_step_into(game, svc_btlengag, Arg(args, count, 2), static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
}

auto port_routine_opeu_battle_supply_notice(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_supply_notice(game, svc_btlengag, Arg(args, count, 1), static_cast<uint8_t>(Arg(args, count, 0)));
  return 0;
}

auto port_routine_opeu_battle_target_in_range(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_target_in_range(game, svc_btlengag, Arg(args, count, 0));
}
