#pragma once
// The six strategic maps' cell layers and the battle fields they lead to.
//   MAPINF.DAT  per cell: terrain class (bits 0..3), side 0 holds it (bit 6), side 1 holds it (bit 7)
//   MAPPNO.DAT  per cell: the tile drawn for it, in both BIGMAP.DAT (8x8) and MIDMAP.DAT (24x24)
//   HEXDISP.DAT per cell: the 1-based battle field of the map, an index into HEXMAP.DAT
//   HEXMAP.DAT  537 battle fields of 14x14 hex cells: terrain code (bits 0..5) and variant (bits 6..7)

#include "opeu/assets/image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace opeu::assets::detail::maps {

inline constexpr auto MAP_COUNT{ std::size_t{ 6 } };
inline constexpr auto MAP_SIDE{ std::size_t{ 40 } };
inline constexpr auto FIELD_SIDE{ std::size_t{ 14 } };
inline constexpr auto FIELD_COUNT{ std::size_t{ 537 } };
inline constexpr auto TILES_PER_MAP{ std::size_t{ 96 } };

using CellGrid    = std::array<std::uint8_t, MAP_SIDE * MAP_SIDE>;     // row-major, cell (y, x) at y * 40 + x
using BattleField = std::array<std::uint8_t, FIELD_SIDE * FIELD_SIDE>; // row-major, cell (ty, tx)

// MAPINF.DAT, MAPPNO.DAT or HEXDISP.DAT: one grid per map.
[[nodiscard]] auto DecodeCellGrids(Bytes bytes) -> Decoded<std::array<CellGrid, MAP_COUNT>>;

[[nodiscard]] auto DecodeBattleFields(Bytes bytes) -> Decoded<std::vector<BattleField>>;

// The first HEXMAP.DAT field of each map; HEXDISP's n on map m is field FIRST_FIELD[m] + n - 1.
inline constexpr auto FIRST_FIELD{
  std::array<unsigned, MAP_COUNT>{ 0, 93, 181, 242, 327, 448 }
};

// The battle's terrain for a stored code (0..42); a larger code is open ground (0).
[[nodiscard]] auto BattleTerrain(std::uint8_t stored_code) noexcept -> std::uint8_t;

}

namespace opeu::assets {
using detail::maps::BattleField;
using detail::maps::BattleTerrain;
using detail::maps::CellGrid;
using detail::maps::DecodeBattleFields;
using detail::maps::DecodeCellGrids;
using detail::maps::FIELD_COUNT;
using detail::maps::FIELD_SIDE;
using detail::maps::FIRST_FIELD;
using detail::maps::MAP_COUNT;
using detail::maps::MAP_SIDE;
using detail::maps::TILES_PER_MAP;
}
