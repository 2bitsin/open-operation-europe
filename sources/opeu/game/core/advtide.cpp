// The staff's "Tide of War" advice: six checks may each give one advice, written into the tide topic's buffer; the
// entry runs them in order until one does.
#include "core/advtide.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/advenemy.hpp"
#include "core/daylist.hpp"
#include "core/sideutil.hpp"
#include "map/cellflag.hpp"
#include "map/cellpred.hpp"
#include "map/query.hpp"
#include "map/reqcells.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t CHEAP_UNIT{ 300 };
constexpr std::uint8_t  SAMPLE_STEP{ 4 }; // the territory sample takes every fourth cell each way

auto Tide(Game& game, std::uint8_t const* far_text) -> Advisor {
  return { game, far_text, AdviceTopic::TIDE_OF_WAR };
}
} // namespace

// One time in three, the first objective of ours without a garrison outside our influence: a city is well isolated
// from the enemy, a lesser objective must be abandoned. Failing that, with no cell to request, one time in three our
// forces are isolated.
uint16_t opeu_advice_tide_isolation(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tide(game, far_text) };
  auto const side{ data.current_side };
  for (auto const& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (!objective.active || opeu_objective_side(game, near) != side) continue;
    auto const garrison{ ObjectiveQuantity(data, near, ANY_CATEGORY).total };
    if (opeu_cell_influence_side(game, objective.x, objective.y) != side && garrison == 0 && advisor.Random(3) == 1)
      return advisor.SayFormatted(opeu_objective_level(game, near) >= CITY_LEVEL ? AdviceText::OBJECTIVE_SAFE : AdviceText::ABANDON_OBJECTIVE, near);
  }
  if (opeu_request_cells(game, side, opeu_request_cell_allowed) == 0 && advisor.Random(3) == 1) return advisor.Say(AdviceText::FORCES_ISOLATED);
  return 0;
}

// Counts the cheap regiments of the active divisions that can act. 12 to 19, one time in four: the buffer is shown as
// it stands (nothing is copied into it); 20 or more, one time in four: we wait for our recovery.
uint16_t opeu_staff_1ad2(Game& game, const uint8_t* far_text) {
  auto&        data{ Data(game) };
  auto const   advisor{ Tide(game, far_text) };
  auto const   side{ data.current_side };
  std::uint8_t cheap{ 0 };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    if (!IsActiveGroup(GroupOf(data, side, g))) continue;
    for (auto const& unit : GroupUnits(data, side, g)) {
      auto const near{ Near(data, unit) };
      if ((unit.flags & Bits(UnitFlag::READY)) && opeu_unit_selection_eligible(game, near) && opeu_unit_value(game, near) <= CHEAP_UNIT) ++cheap;
    }
  }
  if (cheap >= 12 && cheap < 20 && advisor.Random(4) == 1) {
    advisor.Show();
    return 1;
  }
  if (cheap >= 20 && advisor.Random(4) == 1) return advisor.Say(AdviceText::WAIT_FOR_RECOVERY);
  return 0;
}

// Compares both sides' unit counts. Ours above twice the enemy's: go on the offensive. Defending with the enemy's
// above twice ours: wait it out. Attacking with fewer than 30 and a reserve division waiting: wait for the
// reinforcements (the division number is not formatted in). Each one time in four.
uint16_t opeu_advice_tide_strength_balance(Game& game, const uint8_t* far_text) {
  auto&        data{ Data(game) };
  auto const   advisor{ Tide(game, far_text) };
  auto const   side{ data.current_side };
  auto const   foe{ opeu_opposite_side(side) };
  std::uint8_t waiting{ 0 };
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    auto const flags{ GroupOf(data, side, g).flags };
    if ((flags & Bits(GroupFlag::READY)) && (flags & Bits(GroupFlag::RESERVE))) ++waiting;
  }
  auto const ours{ SideQuantity(data, side, ANY_CATEGORY).count };
  auto const theirs{ SideQuantity(data, foe, ANY_CATEGORY).count };
  if (static_cast<std::int16_t>(theirs * 2) < static_cast<std::int16_t>(ours) && advisor.Random(4) == 1) return advisor.Say(AdviceText::GO_ON_OFFENSIVE);
  if (data.side_stance[side] == Bits(Stance::DEFENSIVE) && static_cast<std::int16_t>(ours * 2) < static_cast<std::int16_t>(theirs) && advisor.Random(4) == 1)
    return advisor.Say(AdviceText::WAIT_IT_OUT);
  if (data.side_stance[side] == Bits(Stance::OFFENSIVE) && ours < 30 && waiting != 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::WAIT_FOR_REINFORCEMENTS);
  return 0;
}

// Counts the cells of a 10 x 10 sample of the map under our influence: none, the situation couldn't be worse; over
// 75, we overwhelm the enemy; 1 to 24, the enemy is very powerful; 41 to 59, the enemy is as strong as we are. Each
// one time in four.
uint16_t opeu_advice_tide_territory_control(Game& game, const uint8_t* far_text) {
  auto const&   data{ Data(game) };
  auto const    advisor{ Tide(game, far_text) };
  std::uint16_t held{ 0 };
  for (std::uint8_t y{ 0 }; y < MAP_HEIGHT; y += SAMPLE_STEP)
    for (std::uint8_t x{ 0 }; x < MAP_WIDTH; x += SAMPLE_STEP)
      if (opeu_cell_influence_side(game, x, y) == data.current_side) ++held;
  if (held == 0 && advisor.Random(4) == 1) return advisor.Say(AdviceText::SITUATION_WORST);
  if (held > 75 && advisor.Random(4) == 1) return advisor.Say(AdviceText::OVERWHELMING_ENEMY);
  if (held != 0 && held < 25 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_POWERFUL);
  if (held > 40 && held < 60 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_EQUAL);
  return 0;
}

// The side's air strength: 30 or less, the enemy rules the skies; 70 or more, we do. Each one time in four.
uint16_t opeu_advice_tide_air_superiority(Game& game, const uint8_t* far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Tide(game, far_text) };
  auto const air{ Record<Side>(data, data.current_side_ptr).strength };
  if (air <= 30 && advisor.Random(4) == 1) return advisor.Say(AdviceText::ENEMY_RULES_SKIES);
  if (air >= 70 && advisor.Random(4) == 1) return advisor.Say(AdviceText::WE_RULE_SKIES);
  return 0;
}

// The fallback: there have been no advances.
uint16_t opeu_advice_tide_no_advances(Game& game, const uint8_t* far_text) {
  return Tide(game, far_text).Say(AdviceText::NO_ADVANCES);
}

// Tide of War: runs the checks until one gives advice.
void opeu_advice_tide_of_war(Game& game, const uint8_t* far_text) {
  using Check = uint16_t (*)(Game&, const uint8_t*);
  static constexpr Check CHECKS[]{
    opeu_advice_tide_isolation, opeu_staff_1ad2, opeu_advice_tide_strength_balance, opeu_advice_tide_territory_control, opeu_advice_tide_air_superiority, opeu_advice_tide_no_advances,
  };
  for (auto const check : CHECKS)
    if (check(game, far_text)) return;
}
