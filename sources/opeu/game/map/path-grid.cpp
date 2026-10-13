// The path grid's far memory: the path segment, word-aligned at its paragraph.
#include "map/path-grid.hpp"
#include "common/memory.hpp"

#include "port.hpp"

namespace opeu::game::detail::path_grid {

auto FarPathGrid() -> PathGrid {
  return PathGrid{
    std::span<std::uint16_t, PATH_WORDS>{ reinterpret_cast<std::uint16_t*>(port_far(PATH_SEGMENT, 0)), PATH_WORDS }
  };
}
} // namespace opeu::game::detail::path_grid
