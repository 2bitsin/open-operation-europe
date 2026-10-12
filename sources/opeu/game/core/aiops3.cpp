// The computer side's targets and staff: objective capacities, the side's attack target (an enemy objective or
// special unit), and the rotation of generals too unhealthy to keep their post.
#include "core/aiops3.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "core/aiops1.hpp"
#include "core/counter.hpp"
#include "core/personfl.hpp"
#include "core/random.hpp"
#include "core/sideutil.hpp"
#include "map/query.hpp"
#include "units/resagg.hpp"
#include "units/stats.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr std::uint8_t ENTRIES_PER_SIDE{ 21 };
constexpr std::uint8_t NO_TARGET{ 0xff };
constexpr std::uint8_t TARGET_ORDERED{ 6 }; // the progress of a freshly chosen target
constexpr std::uint8_t FIT_HEALTH{ 30 };    // the least health a general keeps a post with
constexpr std::uint8_t REST_TURNS{ 3 };     // the countdown an unfit general rests for

auto Counter(Person& person) -> std::uint8_t* {
  return reinterpret_cast<std::uint8_t*>(&person);
}

// A general holding a post: a role, or a division.
auto HasPost(Person const& person) -> bool {
  return (person.role & Bits(PersonRole::ROLE_MASK)) || (person.assignment & Bits(Assignment::DIVISION));
}
} // namespace

// Recomputes the reserve capacities of the side's objectives that may be planned for.
uint16_t opeu_ai_refresh_capacities(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  for (auto& objective : data.objectives) {
    auto const near{ Near(data, objective) };
    if (objective.active && opeu_objective_side(ds, near) == data.current_side && AiObjectiveEligible(ds, near))
      opeu_objective_capacities(ds, near, &objective.reserves[0], &objective.reserves[1], &objective.reserves[2]);
  }
  return 1;
}

// A random one of the side's linked objectives held by another side, or 0.
uint16_t opeu_ai_pick_enemy_objective(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t pick{ 0 };
  for (std::uint8_t i{ 0 }; i < ENTRIES_PER_SIDE; ++i) {
    auto const near{ data.objective_links[data.current_side * ENTRIES_PER_SIDE + i] };
    if (near && opeu_objective_side(ds, near) != data.current_side)
      if (!pick || !opeu_rand_bounded(3)) pick = near;
  }
  return pick;
}

// A random ready enemy unit that is special, or 0.
uint16_t opeu_ai_pick_enemy_unit(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t pick{ 0 };
  auto const    enemy{ opeu_opposite_side(data.current_side) };
  for (unsigned i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto&      unit{ data.units[enemy * UNITS_PER_SIDE + i] };
    auto const near{ Near(data, unit) };
    if ((unit.flags & Bits(UnitFlag::READY)) && opeu_unit_special(ds, near))
      if (!pick || !opeu_rand_bounded(8)) pick = near;
  }
  return pick;
}

// When the side has no target: a random draw of 0..7 picks an enemy objective (0, 2, 3, 6) or an enemy unit
// (1, 4, 5, 7); the draw is kept as the order's kind. 1 when a target was chosen.
uint16_t opeu_ai_choose_target(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  auto& order{ Record<Side>(data, data.current_side_ptr).order };
  if (order.kind != NO_TARGET) return 0;
  auto const    draw{ static_cast<std::uint8_t>(opeu_rand_bounded(8)) };
  std::uint16_t target{ 0 };
  switch (draw) {
  case 0:
  case 2:
  case 3:
  case 6:  target = opeu_ai_pick_enemy_objective(ds); break;
  default: target = opeu_ai_pick_enemy_unit(ds); break;
  }
  if (!target) return 0;
  order.target   = target;
  order.kind     = draw;
  order.progress = TARGET_ORDERED;
  return 1;
}

// A general's merit: twice the experience plus leadership, charm, bravery, health and the three arms ratings.
uint16_t opeu_ai_person_merit(uint8_t ds[65536], uint16_t a) {
  auto const& person{ Record<Person>(Data(ds), a) };
  return static_cast<uint16_t>(person.experience * 2 + person.leadership + person.charm + person.bravery + person.health
                               + person.infantry + person.artillery + person.armor);
}

// Lists the side's generals into `list` by descending merit; returns their count.
uint8_t opeu_ai_rank_people(uint8_t ds[65536], uint16_t* list) {
  auto&         data{ Data(ds) };
  std::uint16_t merit[PEOPLE_PER_SIDE];
  std::uint8_t  count{ 0 };
  for (unsigned i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
    auto& person{ data.people[data.current_side * PEOPLE_PER_SIDE + i] };
    if (person.status == PERSON_VACANT) continue;
    list[count]  = Near(data, person);
    merit[count] = opeu_ai_person_merit(ds, list[count]);
    ++count;
  }
  for (std::uint8_t i{ 0 }; i < count; ++i)
    for (std::uint8_t j{ static_cast<std::uint8_t>(count - 1) }; j > i; --j)
      if (merit[j] > merit[j - 1]) {
        std::swap(merit[j], merit[j - 1]);
        std::swap(list[j], list[j - 1]);
      }
  return count;
}

// Hands the post of `person` to the best general without a post, rest countdown or health mark and with health of
// at least 30; `person` loses role and countdown. 1 when someone took the post.
uint16_t opeu_ai_replace_person(uint8_t ds[65536], uint16_t person) {
  auto&         data{ Data(ds) };
  std::uint16_t ranked[PEOPLE_PER_SIDE];
  auto const    count{ opeu_ai_rank_people(ds, ranked) };
  auto&         leaving{ Record<Person>(data, person) };
  for (std::uint8_t i{ 0 }; i < count; ++i) {
    auto& candidate{ Record<Person>(data, ranked[i]) };
    if (candidate.status != PERSON_VACANT && !HasPost(candidate) && !opeu_get_record_counter(Counter(candidate))
        && !(candidate.role & (Bits(PersonRole::POOR_HEALTH) | Bits(PersonRole::DEPUTY)))
        && candidate.health >= FIT_HEALTH) {
      candidate.assignment |= leaving.assignment & (Bits(Assignment::DIVISION) | Bits(Assignment::DIVISION_MASK));
      candidate.role       |= leaving.role & Bits(PersonRole::ROLE_MASK);
      opeu_person_set_role(ds, person, 0);
      opeu_set_record_counter(Counter(leaving), 0);
      return 1;
    }
  }
  return 0;
}

// Replaces the side's generals holding a post whose health is poor (marked, or below 30); a general without a post
// (or just replaced) whose health is poor and who is no deputy rests for three turns. 1 when anything changed.
uint16_t opeu_ai_rotate_staff(uint8_t ds[65536]) {
  auto&         data{ Data(ds) };
  std::uint16_t changed{ 0 };
  for (unsigned i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
    auto& person{ data.people[data.current_side * PEOPLE_PER_SIDE + i] };
    if (person.status == PERSON_VACANT) continue;
    if (HasPost(person)) {
      if (!(person.role & (Bits(PersonRole::POOR_HEALTH) | Bits(PersonRole::DEPUTY))) && person.health >= FIT_HEALTH)
        continue;
      if (!opeu_ai_replace_person(ds, Near(data, person))) continue;
      changed = 1;
    }
    if (!(person.role & Bits(PersonRole::DEPUTY))
        && ((person.role & Bits(PersonRole::POOR_HEALTH)) || person.health < FIT_HEALTH)) {
      opeu_set_record_counter(Counter(person), REST_TURNS);
      changed = 1;
    }
  }
  return changed;
}
