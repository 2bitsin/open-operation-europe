// The end-of-scenario checks: a fallen commander, a side without divisions, the objectives and the deadline.
#pragma once

#include "types.hpp"

// DataSegment::outcome_reason: why the scenario ended (outcome_side holds the winner, 0xff while it goes on).
enum class OutcomeReason : uint8_t {
  COMMANDER_FALLEN = 0, // the loser's army commander fell
  NO_DIVISIONS     = 1, // the loser has no active division left
  OBJECTIVES       = 2, // the winner holds the scenario's objectives
  DEADLINE         = 3, // the scenario's last day came
};

inline constexpr uint8_t NO_OUTCOME{ 0xff }; // outcome_side and outcome_reason while the scenario goes on

void     opeu_set_outcome(uint8_t* ds, uint8_t side, uint8_t reason);
void     opeu_check_sides(uint8_t* ds);
uint8_t  opeu_objective_flag(const uint8_t* ds, uint8_t index);
uint16_t opeu_deadline_components(const uint8_t* ds, uint8_t year, uint8_t month, uint8_t day);
void     opeu_check_objectives(uint8_t* ds);
void     opeu_check_deadline(uint8_t* ds);
uint16_t opeu_check_victory(uint8_t* ds);
