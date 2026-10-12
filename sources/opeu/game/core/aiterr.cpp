// The best terrain rating among a unit's elements at a map cell. Map code 5 rates as code 10. The ratings are a
// [code][column] byte table of five columns; an element type's category selects the column.
#include "types.hpp"
#include "core/aiterr.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "map/grid.hpp"

using namespace opeu::game;

namespace {
constexpr auto RATING_COLUMNS{ 5u };
}

uint8_t opeu_best_terrain(const uint8_t ds[65536], uint8_t x, uint8_t y,
                          uint16_t unit) {
  auto const&  data{ Data(ds) };
  std::uint8_t code{ opeu_map_code(ds, x, y) };
  if (code == 5)
    code = 10;
  std::uint8_t best{ 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const column{
      data.category_columns[data.element_types[element.type].category]
    };
    auto const rating{ data.terrain_ratings[RATING_COLUMNS * code + column] };
    if (rating > best)
      best = rating;
  }
  return best;
}
