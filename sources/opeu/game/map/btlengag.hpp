#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>
#include <optional>

namespace opeu::game::detail::btlengag {

// What a step into a cell did.
enum class StepInto : std::uint8_t {
  BLOCKED = 1, // the cell is taken and it is not the element's goal
  ARRIVED = 2, // the element is at its goal, or out of fuel
  MOVED   = 3,
};

// The verdict on an element's next step toward a cell.
enum class NextStep : std::uint8_t {
  WAIT    = 1, // the order counter has not reached the cheapest free neighbour's cost
  NO_PATH = 2, // no path through free cells: search one ignoring occupants
  STUCK   = 3, // no free neighbour
  FOUND   = 4, // the next step is the cell given
};

struct BattleStep {
  NextStep verdict;
  GridCell cell; // the next step when FOUND, otherwise the destination
};
} // namespace opeu::game::detail::btlengag

namespace opeu::game {
using detail::btlengag::BattleStep;
using detail::btlengag::NextStep;
using detail::btlengag::StepInto;
} // namespace opeu::game

void               opeu_battle_supply_notice(Game& game, uint16_t element, uint8_t supply);
uint16_t           opeu_battle_target_in_range(Game& game, uint16_t element);
uint16_t           opeu_battle_cell_in_range(Game& game, uint16_t element);
void               opeu_battle_animate_facing(Game& game, uint16_t element, uint8_t mode);
[[nodiscard]] auto opeu_battle_step_into(Game& game, uint16_t element, opeu::game::GridCell cell) -> opeu::game::StepInto;
[[nodiscard]] auto opeu_battle_next_step(Game& game, uint16_t element, opeu::game::GridCell destination) -> opeu::game::BattleStep;
// The next step of a path to the goal that may pass occupied cells; none when the goal is out of reach.
[[nodiscard]] auto opeu_battle_element_path_step(Game& game, uint16_t element, opeu::game::GridCell goal) -> std::optional<opeu::game::GridCell>;
