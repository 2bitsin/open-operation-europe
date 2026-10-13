#pragma once

#include "common/events.hpp"
#include "map/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>

int16_t opeu_unit_strength(Game const& game, uint16_t unit);
int16_t opeu_objective_strength(Game const& game, uint16_t objective);
// An influence value with an amount added, clamped to +-20000.
[[nodiscard]] auto opeu_add_influence(std::int16_t value, std::int16_t amount) -> std::int16_t;
// The share of a strength at a source that reaches a square of the influence map.
[[nodiscard]] auto opeu_weight_influence(opeu::game::GridCell square, opeu::game::GridCell source, std::int16_t strength) -> std::int16_t;
