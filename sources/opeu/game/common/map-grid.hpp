// The strategic map's cells and influence squares: map_cells is MAP_WIDTH x MAP_HEIGHT bytes, row by row; the
// influence map holds one signed word per 4 x 4 block of cells.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>

namespace opeu::game::detail::map_grid {

// The map_cells byte of the cell (x, y).
[[nodiscard]] inline auto MapCellAt(DataSegment& data, std::uint8_t x, std::uint8_t y) noexcept -> std::uint8_t& {
  return data.map_cells[y * MAP_WIDTH + x];
}
[[nodiscard]] inline auto MapCellAt(DataSegment const& data, std::uint8_t x, std::uint8_t y) noexcept -> std::uint8_t const& {
  return data.map_cells[y * MAP_WIDTH + x];
}

// The influence word of the block holding the cell (x, y): positive for side 0, negative for side 1.
[[nodiscard]] inline auto InfluenceAt(DataSegment& data, std::uint8_t x, std::uint8_t y) noexcept -> std::int16_t& {
  return data.influence_map[y / 4 * INFLUENCE_WIDTH + x / 4];
}
[[nodiscard]] inline auto InfluenceAt(DataSegment const& data, std::uint8_t x, std::uint8_t y) noexcept -> std::int16_t const& {
  return data.influence_map[y / 4 * INFLUENCE_WIDTH + x / 4];
}

// Whether (x, y) lies on the map; coordinates are bytes, so a step off the top or left edge wraps to a large value.
[[nodiscard]] constexpr auto OnMap(std::uint8_t x, std::uint8_t y) noexcept -> bool {
  return x < MAP_WIDTH && y < MAP_HEIGHT;
}
} // namespace opeu::game::detail::map_grid

namespace opeu::game {
using detail::map_grid::InfluenceAt;
using detail::map_grid::MapCellAt;
using detail::map_grid::OnMap;
} // namespace opeu::game
