// Picks an arrival cell for a side from the candidate grid that the request-cell scan fills. Each candidate after the
// first replaces the pick with probability 1/n, n being the number of candidates.
#include "map/arrplace.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "map/cellpred.hpp"
#include "map/reqcells.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <optional>
#include <ranges>

using namespace opeu::game;

auto ArrivalCell(Game& game, uint8_t side) -> std::optional<GridCell> {
  auto const count{ opeu_request_cells(game, side, opeu_arrival_cell_allowed) };
  if (!count) return std::nullopt;
  auto const&             data{ Data(game) };
  std::optional<GridCell> picked;
  for (auto const cell_y : std::views::iota(uint8_t{ 0 }, MAP_HEIGHT))
    for (auto const cell_x : std::views::iota(uint8_t{ 0 }, MAP_WIDTH))
      if (data.arrival_candidates[cell_y * MAP_WIDTH + cell_x] == 1 && (!picked || game.State().RandomBounded(count) == 0)) picked = GridCell{ cell_x, cell_y };
  return picked;
}
