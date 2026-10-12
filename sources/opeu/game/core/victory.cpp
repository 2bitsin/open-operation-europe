// The end-of-scenario checks. Each check that finds an outcome records it, so a later check overrides an earlier one:
// the deadline wins over the objectives, the objectives over a fallen commander or a side without divisions.
#include "types.hpp"
#include "core/victory.hpp"
#include "core/people.hpp"
#include "common/scenario.hpp"
#include "data-segment.hpp"

#include <utility>

using namespace opeu::game;

namespace {

void SetOutcome(uint8_t* ds, uint8_t side, OutcomeReason reason) {
  opeu_set_outcome(ds, side, std::to_underlying(reason));
}

// A scenario's deadline: years count from 1940.
struct Deadline {
  uint8_t year;
  uint8_t month;
  uint8_t day;
  uint8_t winner; // the side that wins when the deadline comes
};

} // namespace

void opeu_set_outcome(uint8_t ds[65536], uint8_t side, uint8_t reason) {
  auto& data{ Data(ds) };
  data.outcome_side   = side;
  data.outcome_reason = reason;
}

// A side a player commands loses when its army commander has fallen (a deputy holds the post); a side without an
// active division loses.
void opeu_check_sides(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
    if (data.side_status[side] != SIDE_PLAYED)
      continue;
    // A side without the general reads the record at near 0, as the game does.
    auto const& commander{ Record<Person>(
        data, opeu_find_side_record(
                  ds, side, std::to_underlying(StaffRole::ARMY_COMMANDER))) };
    if (commander.role & Bits(PersonRole::DEPUTY))
      SetOutcome(ds, static_cast<uint8_t>(!side),
                 OutcomeReason::COMMANDER_FALLEN);
  }
  for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
    auto active{ false };
    for (uint8_t i = 0; i < GROUPS_PER_SIDE; ++i)
      if (data.groups[side * GROUPS_PER_SIDE + i].flags &
          Bits(GroupFlag::READY)) {
        active = true;
        break;
      }
    if (!active)
      SetOutcome(ds, static_cast<uint8_t>(!side), OutcomeReason::NO_DIVISIONS);
  }
}

// 1 when side 1 holds the objective.
uint8_t opeu_objective_flag(const uint8_t ds[65536], uint8_t index) {
  return (Data(ds).objectives[index].flags & Bits(ObjectiveFlag::SIDE_1)) != 0;
}

// The game compares year, month and day each on their own, not as a date: a June 22 deadline is not met on July 1,
// even of a later year.
uint16_t opeu_deadline_components(const uint8_t ds[65536], uint8_t year,
                                  uint8_t month, uint8_t day) {
  auto const& data{ Data(ds) };
  return year <= data.year && month <= data.month && day <= data.day;
}

// Side 0 wins when side 1 has lost the scenario's key objectives; in Berlin side 1 wins by taking two of them.
void opeu_check_objectives(uint8_t ds[65536]) {
  auto const held{ [ds](uint8_t index) {
    return opeu_objective_flag(ds, index) != 0;
  } };
  switch (CurrentScenario(Data(ds))) {
    case Scenario::FRANCE:
      if (!held(9) && (!held(0) || !held(1)))
        SetOutcome(ds, 0, OutcomeReason::OBJECTIVES);
      break;
    case Scenario::AFRICA:
      if (!held(10) || (!held(12) && !held(13)))
        SetOutcome(ds, 0, OutcomeReason::OBJECTIVES);
      break;
    case Scenario::KURSK:
      if (!held(8) && (!held(5) || !held(10)))
        SetOutcome(ds, 0, OutcomeReason::OBJECTIVES);
      break;
    case Scenario::D_DAY: {
      auto lost{ 0 };
      for (uint8_t index : { 5, 10, 12, 14 })
        if (!held(index))
          ++lost;
      if (lost >= 3 && !held(16) && !held(6) && !held(0))
        SetOutcome(ds, 0, OutcomeReason::OBJECTIVES);
      break;
    }
    case Scenario::BULGE:
      if (!held(0))
        SetOutcome(ds, 0, OutcomeReason::OBJECTIVES);
      break;
    case Scenario::BERLIN:
      if (held(6) && held(0))
        SetOutcome(ds, 1, OutcomeReason::OBJECTIVES);
      break;
  }
}

void opeu_check_deadline(uint8_t ds[65536]) {
  static constexpr Deadline DEADLINES[]{
    { 0, 6, 22, 1 }, // France
    { 2, 7, 5, 1 },  // Africa
    { 3, 9, 5, 1 },  // Kursk
    { 4, 8, 19, 1 }, // D-Day
    { 5, 1, 21, 1 }, // Bulge
    { 5, 5, 16, 0 }, // Berlin
  };
  auto const scenario{ std::to_underlying(CurrentScenario(Data(ds))) };
  if (scenario >= std::size(DEADLINES))
    return;
  auto const& deadline{ DEADLINES[scenario] };
  if (opeu_deadline_components(ds, deadline.year, deadline.month, deadline.day))
    SetOutcome(ds, deadline.winner, OutcomeReason::DEADLINE);
}

// Nonzero when the scenario has ended; outcome_side and outcome_reason say how.
uint16_t opeu_check_victory(uint8_t ds[65536]) {
  auto& data{ Data(ds) };
  data.outcome_side = data.outcome_reason = NO_OUTCOME;
  opeu_check_sides(ds);
  opeu_check_objectives(ds);
  opeu_check_deadline(ds);
  return data.outcome_side != NO_OUTCOME;
}
