// Battle element movement, removal, targeting and the per-round update.
#include "map/btlmove.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/btlcell.hpp"
#include "map/btlelem.hpp"
#include "map/btlicon.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The frame below opeu_battle_move_element's that the element's drawing runs on.
constexpr uint16_t DRAW_FRAME{ 0x12 };
// opeu_battle_attack_rating: the rating of an element already engaged with the target, the odds below which the
// target's own odds stand, and the bonus of a target in the open.
constexpr uint8_t ENGAGED_RATING{ 200 };
constexpr uint8_t LOW_ODDS{ 5 };
constexpr uint8_t OPEN_GROUND_BONUS{ 50 };
constexpr uint8_t OPEN_GROUND{ 1 };
constexpr uint8_t NO_FACING{ 0xff };

[[nodiscard]] auto State(BattleState state) -> uint8_t {
  return std::to_underlying(state);
}

// The element stops where it stands with the new state, no target, no facing and no order.
void Settle(uint8_t* ds, BattleEvent event, uint16_t element, BattleState state) {
  auto& position{ PositionOf(Data(ds), element) };
  position.state  = State(state);
  position.goal_x = position.x;
  position.goal_y = position.y;
  opeu_battle_disengage(ds, event, element);
  opeu_battle_set_facing(ds, event, element, NO_FACING);
  opeu_battle_set_order(ds, event, element, 0);
}
} // namespace

// frame: the routine's frame on the DS stack; the drawing runs below it.
void opeu_battle_move_element(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint8_t to_x, uint8_t to_y,
                              uint16_t element, uint8_t mode, uint16_t frame) {
  if (opeu_battle_element_visible(ds, event, element)) opeu_battle_draw_cell(ds, event, x, y);
  opeu_battle_draw_element_at(ds, event, to_x, to_y, element, mode, Local(frame, DRAW_FRAME));
  opeu_battle_clear_cell(ds, event, x, y);
}

// Unlinks a destroyed element from its unit. An emptied unit is no longer ready and leaves the map; a division left
// without a ready unit is no longer ready either.
void opeu_battle_remove_unit(uint8_t* ds, BattleEvent event, uint16_t unit, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const group{ opeu_unit_group_index(unit) };
  auto const side{ opeu_battle_unit_side(ds, event, unit) };
  // A unit's first and an element's next are both the record's first word.
  for (auto link{ unit }; Word(ds, link); link = Word(ds, link)) {
    if (Word(ds, link) != element) continue;
    auto& removed{ Record<Element>(data, element) };
    SetWord(ds, link, removed.next);
    removed.flags &= static_cast<uint8_t>(~Bits(ElementFlag::READY));
    auto& record{ Record<Unit>(data, unit) };
    if (record.first) return;
    record.flags &= static_cast<uint8_t>(~Bits(UnitFlag::READY));
    if (unit == GARRISON_AT) return;
    MapCellAt(data, record.x, record.y) &= static_cast<uint8_t>(~Bits(MapCell::OCCUPANT_MASK));
    for (auto const& other : GroupUnits(data, side, group))
      if (other.flags & Bits(UnitFlag::READY)) return;
    GroupOf(data, side, group).flags &= static_cast<uint8_t>(~Bits(GroupFlag::READY));
    return;
  }
}

void opeu_battle_remove_list(uint8_t* ds, BattleEvent, uint16_t unit) {
  auto& data{ Data(ds) };
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (BattleList(data, side, slot) == unit) {
        for (auto moved{ slot }; moved < BATTLE_LIST_LENGTH - 1; ++moved)
          BattleList(data, side, moved) = BattleList(data, side, moved + 1);
        BattleList(data, side, BATTLE_LIST_LENGTH - 1) = 0;
        return;
      }
}

// How much the attacker wants to fight the target: the target's odds when they are low, otherwise half of them plus
// the attacker's odds, more for a target in the open.
uint8_t opeu_battle_attack_rating(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target) {
  if (opeu_battle_engaged(ds, event, attacker) && PositionOf(Data(ds), attacker).target == target)
    return ENGAGED_RATING;
  auto const odds{ [&](uint16_t from, uint16_t to) {
    auto const rating{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_RATING, from, to)) };
    auto const range{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_RANGE, from)) };
    return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ODDS, rating, range));
  } };
  auto const defence{ odds(target, attacker) };
  auto const attack{ odds(attacker, target) };
  if (defence < LOW_ODDS) return defence;
  auto rating{ static_cast<uint8_t>((defence >> 1) + attack) };
  if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_TERRAIN_MODE, target, attacker)) == OPEN_GROUND)
    rating = static_cast<uint8_t>(rating + OPEN_GROUND_BONUS);
  return rating;
}

// The spotted enemy element within the element's reach it rates highest, or 0. The original tests the cells with
// the low bytes of its int coordinates after its bounds check.
uint16_t opeu_battle_best_target(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&       data{ Data(ds) };
  auto const  reach{ data.element_types[Record<Element>(data, element).type].mode };
  auto const  side{ opeu_battle_element_side(ds, event, element) };
  auto const& position{ PositionOf(data, element) };
  auto const  span{ static_cast<uint8_t>(reach * 2 + 1) };
  int const   left{ position.x - reach };
  int         y{ position.y - reach };
  uint16_t    best{ 0 };
  uint8_t     best_rating{ 0 };
  for (uint8_t row{ 0 }; row < span; ++row, ++y) {
    int x{ left };
    for (uint8_t column{ 0 }; column < span; ++column, ++x) {
      if (x < 0 || x >= static_cast<int>(BATTLE_WIDTH) || y < 0 || y >= static_cast<int>(BATTLE_WIDTH)
          || !opeu_battle_cell_occupied(ds, event, static_cast<uint8_t>(x), static_cast<uint8_t>(y)))
        continue;
      auto const other{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, static_cast<uint8_t>(x),
                             static_cast<uint8_t>(y)) };
      if (!opeu_battle_element_active(ds, event, other) || opeu_battle_element_side(ds, event, other) == side
          || !opeu_battle_element_flag2(ds, event, other))
        continue;
      if (auto const rating{ opeu_battle_attack_rating(ds, event, element, other) }; rating > best_rating) {
        best_rating = rating;
        best        = other;
      }
    }
  }
  return best_rating ? best : 0;
}

// A defending element waits; a blitzing one waits while its target stands or until it reaches its goal.
uint16_t opeu_battle_can_act(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto const state{ opeu_battle_state(ds, event, element) };
  if (state == State(BattleState::DEFEND)) return 0;
  if (state == State(BattleState::BLITZ)) {
    auto const& position{ PositionOf(Data(ds), element) };
    if (opeu_battle_engaged(ds, event, element)) {
      if (opeu_battle_element_active(ds, event, position.target)) return 0;
    } else if (position.goal_x != position.x || position.goal_y != position.y) {
      return 0;
    }
  }
  return 1;
}

// Without ammunition an element rests and without fuel a vehicle defends; a player's element ready to act defends
// until ordered, and the computer gives its element an order.
void opeu_battle_update_element(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&       data{ Data(ds) };
  auto const  unit{ opeu_battle_element_unit(ds, event, element) };
  auto const& record{ Record<Unit>(data, unit) };
  if (!record.resources[0]) {
    Settle(ds, event, element, BattleState::REST);
  } else if (data.element_types[Record<Element>(data, element).type].weight && !record.resources[1]) {
    Settle(ds, event, element, BattleState::DEFEND);
  } else if (Call(ds, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit)) {
    if (opeu_battle_can_act(ds, event, element)) Settle(ds, event, element, BattleState::DEFEND);
  } else if (opeu_battle_can_act(ds, event, element)
             && !static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN))) {
    data.battle_selected = element;
    Call(ds, event, BattleRoutine::BATTLE_SELECT_SERVICE);
  }
}

uint16_t svc_btlmove(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_remove_list(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_battle_remove_list(ds, svc_btlmove, Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_remove_unit(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_battle_remove_unit(ds, svc_btlmove, Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_battle_update_element(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_battle_update_element(ds, svc_btlmove, Arg(args, count, 0));
  return 0;
}
