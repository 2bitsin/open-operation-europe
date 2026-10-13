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
#include "port.hpp"
#include "services.hpp"
#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
// The supply notice's DS reserve: the unit name buffer and the message buffer, which the services fill.
constexpr uint16_t NOTICE_RESERVE{ 0x88 };
constexpr uint16_t NOTICE_NAME{ 0 };
constexpr uint16_t NOTICE_TEXT{ 0x20 };
constexpr uint8_t  SUPPLY_FUEL{ 1 };
constexpr uint16_t COMMANDER_REPORT{ 2 };
constexpr uint8_t  NO_FACING{ 0xff };
constexpr uint8_t  STEP_DIRECTIONS{ 4 };
constexpr uint16_t PATH_TO_CELL{ 1 };
constexpr uint16_t PATH_TO_GOAL{ 3 };

void Pause(Game& game) {
  opeu_battle_pause(game, static_cast<uint8_t>(2));
}

// Floods the path costs from the destination until the cell is reached; nonzero when it is.
[[nodiscard]] auto FindPath(Game& game, GridCell destination, GridCell cell, uint16_t element, uint16_t mode) -> uint16_t {
  return opeu_battle_path_flood(game, destination.x, destination.y, cell.x, cell.y, element, static_cast<uint8_t>(mode));
}

// The cheapest entry cost of the element's free neighbours; IMPASSABLE_COST when none is free.
[[nodiscard]] auto CheapestNeighbour(Game& game, uint16_t element, GridCell from) -> uint8_t {
  auto const costs{ std::views::iota(uint8_t{ 0 }, STEP_DIRECTIONS) | std::views::transform([&](uint8_t direction) {
                      auto const next{ opeu_battle_step(game, from, direction) };
                      if (!next) return IMPASSABLE_COST;
                      auto const cost{ opeu_battle_move_cost(game, next->x, next->y, element) };
                      return cost == IMPASSABLE_COST || opeu_battle_cell_occupied(game, next->x, next->y) ? IMPASSABLE_COST : cost;
                    }) };
  return std::ranges::min(costs);
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
  return ElementTypeOf(data, element).gun_range < GridDistance({ position.x, position.y }, { position.goal_x, position.goal_y }) ? 0 : 1;
}

// Infantry flash once; other elements turn through the in-between facing to the new one.
void opeu_battle_animate_facing(Game& game, uint16_t element, uint8_t mode) {
  auto&      data{ Data(game) };
  auto const x{ PositionOf(data, element).x };
  auto const y{ PositionOf(data, element).y };
  auto const current{ opeu_battle_mode(game, element) };
  auto const draw{ [&](uint8_t facing) {
    opeu_battle_redraw_cell(game, { x, y }, { element, facing });
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
auto opeu_battle_step_into(Game& game, uint16_t element, GridCell cell) -> StepInto {
  auto&      data{ Data(game) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  if (ElementTypeOf(data, element).fuel_capacity && Record<Unit>(data, unit).resources[1] == 0) {
    opeu_battle_supply_notice(game, element, SUPPLY_FUEL);
    return StepInto::ARRIVED;
  }
  auto const& position{ PositionOf(data, element) };
  if (!opeu_battle_cell_occupied(game, cell.x, cell.y)) {
    uint8_t const facing{
      IsOfCategory(ElementTypeOf(data, element), Category::INFANTRY) ? uint8_t{ 0 }
          : CellFacingToward({ .from = { position.x, position.y }, .to = cell }
          )
    };
    opeu_battle_animate_facing(game, element, facing);
    opeu_battle_move_element(game,
                             {
                                 element, facing
    },
                             { { position.x, position.y }, cell });
    opeu_battle_spot_enemies(game, element);
    opeu_battle_set_order(game, element, 0);
    return StepInto::MOVED;
  }
  opeu_battle_set_facing(game, element, NO_FACING);
  auto const goal{
    opeu_battle_engaged(game, element) ? GridCell{ PositionOf(data, position.target).x, PositionOf(data, position.target).y }
       : GridCell{ position.goal_x, position.goal_y }
  };
  return goal == cell ? StepInto::ARRIVED : StepInto::BLOCKED;
}

// The element waits while its order counter is below the cheapest free neighbour's cost.
auto opeu_battle_next_step(Game& game, uint16_t element, GridCell destination) -> BattleStep {
  auto const&    position{ PositionOf(Data(game), element) };
  GridCell const from{ position.x, position.y };
  auto const     cheapest{ CheapestNeighbour(game, element, from) };
  if (cheapest == IMPASSABLE_COST) return { NextStep::STUCK, destination };
  if (opeu_battle_order(game, element) < cheapest) return { NextStep::WAIT, destination };
  if (!FindPath(game, destination, from, element, PATH_TO_CELL)) return { NextStep::NO_PATH, destination };
  return { NextStep::FOUND, DownhillStep(game, from).cell };
}

auto opeu_battle_element_path_step(Game& game, uint16_t element, GridCell goal) -> std::optional<GridCell> {
  auto const&    position{ PositionOf(Data(game), element) };
  GridCell const from{ position.x, position.y };
  if (!FindPath(game, goal, from, element, PATH_TO_GOAL)) return std::nullopt;
  return DownhillStep(game, from).cell;
}
