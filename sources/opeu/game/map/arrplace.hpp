// The arrival cell of a side's reinforcements (map/arrplace.cpp).
#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <optional>

// A random cell among the side's arrival candidates; none when there is no candidate.
[[nodiscard]] auto ArrivalCell(Game& game, uint8_t side) -> std::optional<opeu::game::GridCell>;
