// The strategic map's path search scratch in the far path segment: a queue of cell indices kept sorted by cost
// (from word 0), then a 40 x 40 grid of accumulated costs (from word 0x640), 0xffff for a cell not reached.
#pragma once

#include "common/data-segment.hpp"
#include "map/grid-cell.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::game::detail::path_grid {

inline constexpr std::size_t PATH_QUEUE_WORDS{ 0x640 };
inline constexpr std::size_t PATH_CELLS{ MAP_WIDTH * MAP_HEIGHT };
inline constexpr std::size_t PATH_WORDS{ PATH_QUEUE_WORDS + PATH_CELLS };

class PathGrid {
public:
  static constexpr std::uint16_t UNREACHED{ 0xffff };

  explicit PathGrid(std::span<std::uint16_t, PATH_WORDS> words) noexcept : _words{ words } { }

  // The queue runs on into the cost grid, as the segment lays them out.
  [[nodiscard]] auto Queue() const noexcept -> std::span<std::uint16_t, PATH_WORDS> {
    return _words;
  }
  [[nodiscard]] auto Costs() const noexcept -> std::span<std::uint16_t, PATH_CELLS> {
    return _words.last<PATH_CELLS>();
  }
  [[nodiscard]] auto Cost(GridCell cell) const -> std::uint16_t {
    return Costs()[Index(cell)];
  }
  // Marks the cell as not reached.
  auto Clear(GridCell cell) const -> void {
    Costs()[Index(cell)] = UNREACHED;
  }
  [[nodiscard]] static constexpr auto Index(GridCell cell) noexcept -> std::uint16_t {
    return static_cast<std::uint16_t>(cell.y * MAP_WIDTH + cell.x);
  }

private:
  std::span<std::uint16_t, PATH_WORDS> _words;
};

// The path grid in the far path segment.
[[nodiscard]] auto FarPathGrid() -> PathGrid;
} // namespace opeu::game::detail::path_grid

namespace opeu::game {
using detail::path_grid::FarPathGrid;
using detail::path_grid::PATH_CELLS;
using detail::path_grid::PATH_QUEUE_WORDS;
using detail::path_grid::PATH_WORDS;
using detail::path_grid::PathGrid;
} // namespace opeu::game
