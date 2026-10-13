// The computer side's targets and staff: objective capacities, the side's attack target (an enemy objective or
// special unit), and the rotation of generals too unhealthy to keep their post.
#include "core/aiops3.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/aiops1.hpp"
#include "core/counter.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "map/query.hpp"
#include "units/resagg.hpp"
#include "units/stats.hpp"

#include <algorithm>
#include <functional>
#include <ranges>
#include <utility>
#include <vector>

using namespace opeu::game;

namespace {

constexpr std::uint8_t NO_TARGET{ 0xff };
constexpr std::uint8_t TARGET_ORDERED{ 6 }; // the progress of a freshly chosen target
constexpr std::uint8_t FIT_HEALTH{ 30 };    // the least health a general keeps a post with
constexpr std::uint8_t REST_TURNS{ 3 };     // the countdown an unfit general rests for

// A general holding a post: a role, or a division.
auto HasPost(Person const& person) -> bool {
  return (person.role & Bits(PersonRole::ROLE_MASK)) || (person.assignment & Bits(Assignment::DIVISION));
}

auto Present(Person const& person) -> bool {
  return person.status != PERSON_VACANT;
}

// A general's merit: twice the experience plus leadership, charm, bravery, health and the three arms ratings.
auto Merit(Person const& person) -> std::uint16_t {
  return static_cast<std::uint16_t>(person.experience * 2 + person.leadership + person.charm + person.bravery + person.health + person.infantry + person.artillery + person.armor);
}

// The current side's present generals by descending merit (equal merits keep their order).
auto RankedGenerals(DataSegment& data) -> std::vector<std::reference_wrapper<Person>> {
  auto ranked{ SideGenerals(data, data.current_side) | std::views::filter(Present) | std::ranges::to<std::vector<std::reference_wrapper<Person>>>() };
  std::ranges::stable_sort(ranked, std::ranges::greater{}, [](Person const& person) { return Merit(person); });
  return ranked;
}

// A general who can take a post: none held, no rest countdown, no poor-health or deputy mark, health at least 30.
auto CanTakePost(Person const& person) -> bool {
  return !HasPost(person) && !opeu_get_record_counter(person) && !(person.role & (Bits(PersonRole::POOR_HEALTH) | Bits(PersonRole::DEPUTY))) && person.health >= FIT_HEALTH;
}

// Hands the post of `leaving` to the best general who can take it; `leaving` loses role and countdown. Whether
// someone took the post.
auto ReplacePerson(Game& game, Person& leaving) -> bool {
  auto&      data{ Data(game) };
  auto const ranked{ RankedGenerals(data) };
  auto const successor{ std::ranges::find_if(ranked, CanTakePost) };
  if (successor == ranked.end()) return false;
  auto& candidate{ successor->get() };
  candidate.assignment |= leaving.assignment & (Bits(Assignment::DIVISION) | Bits(Assignment::DIVISION_MASK));
  candidate.role       |= leaving.role & Bits(PersonRole::ROLE_MASK);
  opeu_person_set_role(game, Near(data, leaving), 0);
  opeu_set_record_counter(leaving, 0);
  return true;
}
} // namespace

// Recomputes the reserve capacities of the side's objectives that may be planned for.
uint16_t opeu_ai_refresh_capacities(Game& game) {
  auto& data{ Data(game) };
  for (auto& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active && opeu_objective_side(game, near) == data.current_side && AiObjectiveEligible(game, near))
      std::ranges::copy(Figures(ObjectiveCapacities(data, near)), std::ranges::begin(objective.reserves));
  }
  return 1;
}

// A random one of the side's linked objectives held by another side, or 0.
uint16_t opeu_ai_pick_enemy_objective(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t pick{ 0 };
  for (std::uint8_t i{ 0 }; i < OBJECTIVE_LINKS_PER_SIDE; ++i) {
    auto const near{ data.objective_links[data.current_side * OBJECTIVE_LINKS_PER_SIDE + i] };
    if (near && opeu_objective_side(game, near) != data.current_side)
      if (!pick || !game.State().Generator().Bounded(3)) pick = near;
  }
  return pick;
}

// A random ready enemy unit that is special, or 0.
uint16_t opeu_ai_pick_enemy_unit(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t pick{ 0 };
  auto const    enemy{ opeu_opposite_side(data.current_side) };
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto&      unit{ data.units[enemy * UNITS_PER_SIDE + i] };
    auto const near{ Near(data, unit) };
    if ((unit.flags & Bits(UnitFlag::READY)) && opeu_unit_special(game, near))
      if (!pick || !game.State().Generator().Bounded(8)) pick = near;
  }
  return pick;
}

// When the side has no target: a random draw of 0..7 picks an enemy objective (0, 2, 3, 6) or an enemy unit
// (1, 4, 5, 7); the draw is kept as the order's kind. 1 when a target was chosen.
uint16_t opeu_ai_choose_target(Game& game) {
  auto& data{ Data(game) };
  auto& order{ Record<Side>(data, data.current_side_ptr).order };
  if (order.kind != NO_TARGET) return 0;
  auto const    draw{ static_cast<std::uint8_t>(game.State().Generator().Bounded(8)) };
  std::uint16_t target{ 0 };
  switch (draw) {
  case 0:
  case 2:
  case 3:
  case 6:  target = opeu_ai_pick_enemy_objective(game); break;
  default: target = opeu_ai_pick_enemy_unit(game); break;
  }
  if (!target) return 0;
  order.target   = target;
  order.kind     = draw;
  order.progress = TARGET_ORDERED;
  return 1;
}

// Replaces the side's generals holding a post whose health is poor (marked, or below 30); a general without a post
// (or just replaced) whose health is poor and who is no deputy rests for three turns. 1 when anything changed.
uint16_t opeu_ai_rotate_staff(Game& game) {
  auto&         data{ Data(game) };
  std::uint16_t changed{ 0 };
  for (auto& person : SideGenerals(data, data.current_side) | std::views::filter(Present)) {
    if (HasPost(person)) {
      if (!(person.role & (Bits(PersonRole::POOR_HEALTH) | Bits(PersonRole::DEPUTY))) && person.health >= FIT_HEALTH) continue;
      if (!ReplacePerson(game, person)) continue;
      changed = 1;
    }
    if (!(person.role & Bits(PersonRole::DEPUTY)) && ((person.role & Bits(PersonRole::POOR_HEALTH)) || person.health < FIT_HEALTH)) {
      opeu_set_record_counter(person, REST_TURNS);
      changed = 1;
    }
  }
  return changed;
}
