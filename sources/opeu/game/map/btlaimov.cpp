// The computer's battle unit profile and its elements' move and attack orders.
//
// The attack keeps its two coordinate bytes in its reserve, where the random cell choice writes them.
#include "map/btlaimov.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlaiunt.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "map/btlaiact.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 0xff };
// ElementType::speed above this makes fast tanks.
constexpr uint8_t FAST_SPEED{ 60 };

// The original's counters frame: the six kind counters, then the loop index, the element cursor and, at 10, the
// element count, which battle_ai_kind reaches for category 6.
constexpr unsigned KIND_COUNT{ 6 };
constexpr unsigned FAST_TANKS{ 1 };
constexpr unsigned INFANTRY_KIND{ 2 }; // from battle_ai_kind
constexpr unsigned SPECIAL_KIND{ 4 };  // from battle_ai_kind
constexpr unsigned RANGED{ 5 };
constexpr unsigned ELEMENT_COUNT{ 10 };
} // namespace

// The profile of the unit's elements (the spotted ones, or all of them), indexing battle_ai_actions.
uint8_t opeu_battle_ai_profile(Game& game, uint16_t unit, uint16_t all) {
  auto&                   data{ Data(game) };
  std::array<uint8_t, 12> counters{};
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    if (!opeu_battle_element_active(game, Near(data, element))) continue;
    if ((element.flags & Bits(ElementFlag::SPOTTED)) || all) {
      auto const& type{ data.element_types[element.type] };
      if (type.gun_range > RANGED_MODE)
        ++counters[RANGED];
      else if (type.speed > FAST_SPEED && type.category == Bits(Category::TANKS))
        ++counters[FAST_TANKS];
      else {
        auto const kind{ Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_ai_kind) + type.category)) };
        if (kind < KIND_COUNT || kind == ELEMENT_COUNT) ++counters[kind];
      }
    }
    ++counters[ELEMENT_COUNT];
  }
  auto const count{ counters[ELEMENT_COUNT] };
  if (counters[SPECIAL_KIND]) return 4;
  if (counters[RANGED] && count - counters[RANGED] < 4) return 2;
  if (count * 2 / 3 <= counters[INFANTRY_KIND] || counters[INFANTRY_KIND] > 2) return 0;
  if (count * 2 / 3 <= counters[FAST_TANKS] || counters[FAST_TANKS] > 2) return 1;
  return 3;
}

auto opeu_battle_ai_move_to(Game& game, GridCell cell, uint16_t element) -> void {
  auto& position{ PositionOf(Data(game), element) };
  position.goal_x = cell.x;
  position.goal_y = cell.y;
  position.state  = Bits(BattleState::BLITZ);
  opeu_battle_disengage(game, element);
  opeu_battle_set_facing(game, element, NONE);
}

void opeu_battle_ai_attack(Game& game, uint16_t element, uint16_t unit, uint16_t target) {
  if (target) {
    auto& position{ PositionOf(Data(game), element) };
    position.target = target;
    position.state  = Bits(BattleState::BLITZ);
    opeu_battle_engage(game, element);
    opeu_battle_set_facing(game, element, NONE);
    return;
  }
  auto const cell{ opeu_battle_ai_random_cell(game, element, unit) };
  opeu_battle_ai_move_to(game, cell, element);
}

void opeu_battle_ai_attack_near(Game& game, uint16_t element, uint16_t unit) {
  opeu_battle_ai_attack(game, element, unit, opeu_battle_ai_pick_target(game, element, unit, 0));
}

void opeu_battle_ai_attack_mid(Game& game, uint16_t element, uint16_t unit) {
  opeu_battle_ai_attack(game, element, unit, opeu_battle_ai_pick_target(game, element, unit, 1));
}

void opeu_battle_ai_attack_far(Game& game, uint16_t element, uint16_t unit) {
  opeu_battle_ai_attack(game, element, unit, opeu_battle_ai_pick_target(game, element, unit, 2));
}
