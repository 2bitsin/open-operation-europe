// Battle turn control: the outcome test, the computer's turns, the player's input loop, the garrison's return and
// the elements' daily condition.
#include "map/btlturn.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/keys.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "map/btlelem.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// opeu_battle_computer_turn's verdict.
constexpr uint8_t BATTLE_GOES_ON{ 0 };
constexpr uint8_t SIDE_0_DECIDES{ 1 };
constexpr uint8_t SIDE_1_WINS{ 2 };
constexpr uint8_t TURN_STEPS{ 50 };
constexpr uint8_t NO_WINNER{ 0xff };
// MOUSE_BUTTONS: a pending event, the left and the right button.
constexpr uint8_t MOUSE_EVENT{ 0x80 };
constexpr uint8_t LEFT_BUTTON{ 1 };
constexpr uint8_t RIGHT_BUTTON{ 2 };
constexpr uint8_t BRIDGE_TERRAIN{ 10 };

// Calls body(unit, element) for each element of each unit in the battle's lists.
template <typename Body>
void ForBattleElements(DataSegment& data, Body&& body) {
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, side, slot) })
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) body(unit, *it);
}
} // namespace

uint8_t opeu_battle_computer_turn(Game& game, BattleEvent) {
  auto&   data{ Data(game) };
  uint8_t present{ 0 };
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, 1, slot) }) {
      auto const& record{ Record<Unit>(data, unit) };
      if (record.x == data.battle_x && record.y == data.battle_y) ++present;
    }
  if (!present) return SIDE_0_DECIDES;
  if (!BattleList(data, 0, 0)) return SIDE_1_WINS;
  return BATTLE_GOES_ON;
}

uint16_t opeu_battle_unit_turn(Game& game, BattleEvent event, uint16_t unit) {
  auto& data{ Data(game) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (opeu_battle_element_active(game, event, it.Near())) {
      Call(game, event, BattleRoutine::BATTLE_ELEMENT_STEP, it.Near());
      if (opeu_battle_computer_turn(game, event)) return 0;
    }
  return 1;
}

uint16_t opeu_battle_all_turns(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, side, slot) }; unit && !opeu_battle_unit_turn(game, event, unit)) return 0;
  return 1;
}

uint16_t opeu_battle_side_controlled(Game& game, BattleEvent event, uint8_t side) {
  auto& data{ Data(game) };
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, side, slot) }; unit && Call(game, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit)) return 1;
  return 0;
}

// Shows or hides the element numbers and redraws the cells.
void opeu_battle_toggle_numbers(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  data.battle_numbers_shown = data.battle_numbers_shown == 1 ? 0 : 1;
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x)
      if (opeu_battle_cell_occupied(game, event, x, y) || static_cast<uint8_t>(Call(game, event, BattleRoutine::BATTLE_CELL_TERRAIN, x, y)) == BRIDGE_TERRAIN)
        opeu_battle_refresh_cell(game, event, x, y);
}

// frame: the routine's frame; its callees run below it. One path (a mouse event with the keyboard in use) reads the
// key before any key is stored: it starts as 0.
uint16_t opeu_battle_turn_loop(Game& game, BattleEvent event) {
  auto&   data{ Data(game) };
  uint8_t key{ 0 };
  for (uint8_t step{ 0 }; step < TURN_STEPS; ++step) {
    if (!data.battle_level_limit && !step) {
      Call(game, event, BattleRoutine::BATTLE_PLAYER_TURN);
    } else {
      auto       buttons{ static_cast<uint8_t>(Call(game, event, BattleRoutine::MOUSE_BUTTONS)) };
      auto const input{ [&] {
        if (buttons & MOUSE_EVENT) return true;
        if (!Call(game, event, BattleRoutine::TURN_INPUT_B)) return false;
        key = static_cast<uint8_t>(Call(game, event, BattleRoutine::GETCH));
        return key != 0;
      } };
      if (input()) {
        if (data.input_mode)
          key = 0;
        else
          buttons = 0;
        if ((buttons & LEFT_BUTTON) || key == KEY_ENTER)
          if (opeu_battle_side_controlled(game, event, 0) || opeu_battle_side_controlled(game, event, 1)) Call(game, event, BattleRoutine::BATTLE_PLAYER_TURN);
        if ((buttons & RIGHT_BUTTON) || key == '0' || key == KEY_ESCAPE) opeu_battle_toggle_numbers(game, event);
      }
    }
    Call(game, event, BattleRoutine::INPUT_RESET);
    if (!opeu_battle_all_turns(game, event)) return 0;
  }
  return 1;
}

// The garrison's supplies and surviving equipment go back to the objective, up to three times each type's capacity.
void opeu_battle_return_garrison(Game& game, BattleEvent) {
  auto&      data{ Data(game) };
  auto const objective_at{ opeu_find_objective_at(game, data.battle_x, data.battle_y) };
  auto const unit{ BattleList(data, 1, 0) };
  if (!unit || unit != GARRISON_AT) return;
  auto& objective{ Record<Objective>(data, objective_at) };
  auto& garrison{ Record<Unit>(data, unit) };
  objective.resources[1] = static_cast<uint16_t>(objective.resources[1] + garrison.resources[1]);
  objective.resources[0] = static_cast<uint16_t>(objective.resources[0] + garrison.resources[0]);
  for (auto const& element : Elements(data, garrison)) {
    auto const capacity{ data.request_capacities[data.element_types[element.type].category] };
    uint16_t   left{ element.quantity };
    for (auto& item : objective.items) {
      if (item.type != element.type) continue;
      auto const returned{ opeu_min16(left, static_cast<uint16_t>(capacity * 3u - item.quantity)) };
      item.quantity = static_cast<uint16_t>(item.quantity + returned);
      left          = static_cast<uint16_t>(left - returned);
      uint16_t extra{ item.extra };
      opeu_add_word(element.damage, capacity, &extra);
      item.extra = extra;
      if (!left) break;
    }
  }
}

// Morale follows the battle's winner; fatigue and experience grow, more at night.
void opeu_battle_daily_condition(Game& game, BattleEvent event) {
  auto& data{ Data(game) };
  ForBattleElements(data, [&](uint16_t unit, Element& element) {
    if (data.battle_side != NO_WINNER) {
      if (opeu_battle_unit_side(game, event, unit) == data.battle_side)
        opeu_add_byte(1, RATING_LIMIT, &element.morale);
      else
        opeu_sub_byte(1, &element.morale);
    }
    auto const round{ static_cast<uint16_t>(data.battle_level_limit + 1) };
    if (!opeu_hour_is_night(data.hour)) {
      opeu_add_byte(1, RATING_LIMIT, &element.training);
      opeu_add_byte(opeu_min16(opeu_rand_bounded(round), 5), RATING_LIMIT, &element.experience);
    } else {
      opeu_sub_byte(1, &element.morale);
      opeu_add_byte(1, RATING_LIMIT, &element.training);
      opeu_add_byte(opeu_min16(static_cast<uint16_t>(opeu_rand_bounded(round) + 5), 10), RATING_LIMIT, &element.experience);
    }
  });
}

void opeu_battle_clear_positions(Game& game, BattleEvent) {
  ForBattleElements(Data(game), [](uint16_t, Element& element) { element.position = 0; });
}

uint16_t svc_btlturn(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_clear_positions(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_clear_positions(game, svc_btlturn);
  return 0;
}

auto port_routine_opeu_battle_computer_turn(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_battle_computer_turn(game, svc_btlturn);
}

auto port_routine_opeu_battle_daily_condition(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_daily_condition(game, svc_btlturn);
  return 0;
}

auto port_routine_opeu_battle_return_garrison(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_return_garrison(game, svc_btlturn);
  return 0;
}

auto port_routine_opeu_battle_turn_loop(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_battle_turn_loop(game, svc_btlturn);
}
