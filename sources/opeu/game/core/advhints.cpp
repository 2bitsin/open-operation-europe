// The staff's "Operation Hints", the fifth entry of the advice menu: each hint is given with a chance of one in four
// (the last one in six); the first that fires writes the hints topic's buffer and shows it. The fallback shows one of
// the twenty general tips.
#include "core/advhints.hpp"
#include "common/events.hpp"
#include "types.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/advenemy.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "port.hpp"

#include <span>

using namespace opeu::game;

namespace {

constexpr std::uint8_t STAFF_TEMPLATE{ 6 }; // the person template kind of the air commander

auto Hints(Game& game, std::span<std::uint8_t const> far_text) -> Advisor {
  return { game, far_text, AdviceTopic::HINTS };
}
} // namespace

// Hints on the side's air strength (low, high, middle), on reserve divisions 3 and 4, on a division led by a deputy,
// on an intelligence chief of little charm (he and the air commander disagree), then on the scenario: the Ardennes
// winter (scenario 4), the hurricane season (scenario 3 in July to September), and bridges in any scenario but 1 and
// 2, worded by the side's stance.
uint16_t opeu_advice_situational_hint(Game& game, std::span<std::uint8_t const> far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Hints(game, far_text) };
  auto const side{ data.current_side };
  auto const air{ Record<Side>(data, data.current_side_ptr).strength };
  if (air <= 30 && advisor.Random(4) == 1) return advisor.Say(AdviceText::HINT_AIR_SUPPORT);
  if (air >= 50 && advisor.Random(4) == 1) return advisor.Say(AdviceText::HINT_CARPET_BOMBING);
  if (air >= 30 && advisor.Random(4) == 1) return advisor.Say(AdviceText::HINT_AIR_GUNFIRE);
  for (std::uint8_t g{ 2 }; g < GROUPS_PER_SIDE; ++g)
    if ((GroupOf(data, side, g).flags & Bits(GroupFlag::RESERVE)) && advisor.Random(4) == 1) return advisor.SayFormatted(AdviceText::HINT_RESERVE_DIVISION, g + 1);
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    auto const person{ opeu_find_group_record(game, side, g) };
    if (person && (Record<Person>(data, person).role & Bits(PersonRole::DEPUTY)) && advisor.Random(4) == 1) return advisor.SayFormatted(AdviceText::HINT_DEPUTY, g + 1);
  }
  if (Record<Person>(data, data.context_record3).charm <= 40 && advisor.Random(4) == 1)
    return advisor.SayFormatted(AdviceText::HINT_STAFF_DISAGREE, data.context_record3, opeu_find_person_template(game, Record<Person>(data, data.context_record1).status, STAFF_TEMPLATE));
  auto const scenario{ CurrentScenario(data) };
  if (scenario == Scenario::BULGE && advisor.Random(4) == 1) return advisor.Say(AdviceText::HINT_ARDENNES_WINTER);
  if (scenario == Scenario::D_DAY && data.month >= 7 && data.month <= 9 && advisor.Random(4) == 1) return advisor.Say(AdviceText::HINT_HURRICANE_SEASON);
  if (scenario != Scenario::AFRICA && scenario != Scenario::KURSK && advisor.Random(6) == 1)
    return advisor.Say(data.side_stance[side] == Bits(Stance::OFFENSIVE) ? AdviceText::HINT_BUILD_BRIDGE : AdviceText::HINT_DESTROY_BRIDGES);
  return 0;
}

// One of the twenty general tips.
uint16_t opeu_advice_general_tip(Game& game, std::span<std::uint8_t const> far_text) {
  auto&      data{ Data(game) };
  auto const advisor{ Hints(game, far_text) };
  auto const tip{ advisor.Random(20) };
  opeu_string_copy_far(game, advisor.Buffer(), far_text.subspan(data.advice_tips[tip].offset));
  advisor.Show();
  return 1;
}

// Operation Hints: a situational hint, else a general tip.
uint16_t opeu_advice_operation_hints(Game& game, std::span<std::uint8_t const> far_text) {
  if (auto const given{ opeu_advice_situational_hint(game, far_text) }) return given;
  return opeu_advice_general_tip(game, far_text);
}
