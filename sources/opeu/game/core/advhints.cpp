// The staff's "Operation Hints", the fifth entry of the advice menu: each hint is given with a chance of one in four
// (the last one in six); the first that fires writes the hints topic's buffer and shows it. The fallback shows one of
// the twenty general tips.
#include "types.hpp"
#include "core/advhints.hpp"
#include "common/events.hpp"

#include "common/advice.hpp"
#include "common/data-segment.hpp"
#include "common/scenario.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/advenemy.hpp"
#include "core/people.hpp"
#include "core/template.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint8_t STAFF_TEMPLATE{
  6
}; // the person template kind of the air commander

auto Hints(std::uint8_t* ds, HostEvent event, std::uint8_t const* far_text,
           std::uint16_t sp) -> Advisor {
  return { ds, far_text, event, AdviceTopic::HINTS, sp };
}
} // namespace

// Hints on the side's air strength (low, high, middle), on reserve divisions 3 and 4, on a division led by a deputy,
// on an intelligence chief of little charm (he and the air commander disagree), then on the scenario: the Ardennes
// winter (scenario 4), the hurricane season (scenario 3 in July to September), and bridges in any scenario but 1 and
// 2, worded by the side's stance.
uint16_t opeu_advice_situational_hint(uint8_t        ds[65536],
                                      const uint8_t* far_text, HostEvent event,
                                      uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Hints(ds, event, far_text, sp) };
  auto const side{ data.current_side };
  auto const text{ advisor.Scratch(0x68) };
  auto const air{ Record<Side>(data, data.current_side_ptr).strength };
  if (air <= 30 && advisor.Random(4) == 1)
    return advisor.Say(AdviceText::HINT_AIR_SUPPORT);
  if (air >= 50 && advisor.Random(4) == 1)
    return advisor.Say(AdviceText::HINT_CARPET_BOMBING);
  if (air >= 30 && advisor.Random(4) == 1)
    return advisor.Say(AdviceText::HINT_AIR_GUNFIRE);
  for (std::uint8_t g{ 2 }; g < GROUPS_PER_SIDE; ++g)
    if ((GroupOf(data, side, g).flags & Bits(GroupFlag::RESERVE)) &&
        advisor.Random(4) == 1)
      return advisor.SayFormatted(text, AdviceText::HINT_RESERVE_DIVISION,
                                  g + 1);
  for (std::uint8_t g{ 0 }; g < GROUPS_PER_SIDE; ++g) {
    auto const person{ opeu_find_group_record(ds, side, g) };
    if (person &&
        (Record<Person>(data, person).role & Bits(PersonRole::DEPUTY)) &&
        advisor.Random(4) == 1)
      return advisor.SayFormatted(text, AdviceText::HINT_DEPUTY, g + 1);
  }
  if (Record<Person>(data, data.context_record3).charm <= 40 &&
      advisor.Random(4) == 1)
    return advisor.SayFormatted(
        text, AdviceText::HINT_STAFF_DISAGREE, data.context_record3,
        opeu_find_person_template(
            ds, Record<Person>(data, data.context_record1).status,
            STAFF_TEMPLATE));
  auto const scenario{ CurrentScenario(data) };
  if (scenario == Scenario::BULGE && advisor.Random(4) == 1)
    return advisor.Say(AdviceText::HINT_ARDENNES_WINTER);
  if (scenario == Scenario::D_DAY && data.month >= 7 && data.month <= 9 &&
      advisor.Random(4) == 1)
    return advisor.Say(AdviceText::HINT_HURRICANE_SEASON);
  if (scenario != Scenario::AFRICA && scenario != Scenario::KURSK &&
      advisor.Random(6) == 1)
    return advisor.Say(data.side_stance[side] == Bits(Stance::OFFENSIVE)
                           ? AdviceText::HINT_BUILD_BRIDGE
                           : AdviceText::HINT_DESTROY_BRIDGES);
  return 0;
}

// One of the twenty general tips.
uint16_t opeu_advice_general_tip(uint8_t ds[65536], const uint8_t* far_text,
                                 HostEvent event, uint16_t sp) {
  auto&      data{ Data(ds) };
  auto const advisor{ Hints(ds, event, far_text, sp) };
  auto const tip{ advisor.Random(20) };
  CopyAdvice(ds, far_text, advisor.Buffer(), data.advice_tips[tip].offset);
  advisor.Show();
  return 1;
}

// Operation Hints: a situational hint, else a general tip.
uint16_t opeu_advice_operation_hints(uint8_t ds[65536], const uint8_t* far_text,
                                     HostEvent event, uint16_t sp) {
  auto const nested{ static_cast<std::uint16_t>(sp - 4) };
  if (auto const given{
          opeu_advice_situational_hint(ds, far_text, event, nested) })
    return given;
  return opeu_advice_general_tip(ds, far_text, event, nested);
}

uint16_t svc_advhints(uint8_t* ds, uint16_t id, const uint16_t* args,
                      uint16_t count) {
  return svc_advice("advhints", ds, id, args, count);
}

auto port_routine_opeu_advice_operation_hints(std::uint8_t* ds,
                                              std::uint16_t const*, unsigned)
    -> std::uint32_t {
  return WithStack(PORT_FRAME, [&](std::uint16_t sp) {
    return opeu_advice_operation_hints(ds, AdviceTexts(), svc_advhints, sp);
  });
}
