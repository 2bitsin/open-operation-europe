#include "opeu/assets/maps.hpp"

#include <algorithm>
#include <ranges>

namespace opeu::assets::detail::maps {

namespace {

constexpr auto TERRAIN{
  std::array<std::uint8_t, 43>{ 0, 1, 2, 3, 4, 17, 18, 19, 20, 14, 15, 16, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 5, 6, 7, 8, 9, 10, 11, 12 }
};

template <class Record>
auto Values(Bytes const record) -> Record {
  auto values{ Record{} };
  std::ranges::transform(record, values.begin(), [](std::byte const b) { return std::to_integer<std::uint8_t>(b); });
  return values;
}

}

auto DecodeCellGrids(Bytes const bytes) -> Decoded<std::array<CellGrid, MAP_COUNT>> {
  if (bytes.size() != MAP_COUNT * MAP_SIDE * MAP_SIDE) return std::unexpected{ DecodeError::BAD_SIZE };
  auto grids{ std::array<CellGrid, MAP_COUNT>{} };
  std::ranges::transform(Chunks(bytes, MAP_SIDE * MAP_SIDE), grids.begin(), Values<CellGrid>);
  return grids;
}

auto DecodeBattleFields(Bytes const bytes) -> Decoded<std::vector<BattleField>> {
  if (bytes.size() != FIELD_COUNT * FIELD_SIDE * FIELD_SIDE) return std::unexpected{ DecodeError::BAD_SIZE };
  return Chunks(bytes, FIELD_SIDE * FIELD_SIDE) | std::views::transform(Values<BattleField>) | std::ranges::to<std::vector>();
}

auto BattleTerrain(std::uint8_t const stored_code) noexcept -> std::uint8_t {
  return stored_code < TERRAIN.size() ? TERRAIN[stored_code] : 0;
}

}
