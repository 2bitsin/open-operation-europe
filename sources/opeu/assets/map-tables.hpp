#pragma once
// The map layers and battle fields as tables (docs/asset-pack.md, "Tables"): tables/maps.json holds each map's
// MAPINF, MAPPNO and HEXDISP grid with MAPINF's bits as named grids, tables/battle-fields.json the 537 HEXMAP fields
// with their terrain code and variant as two grids.

#include "opeu/assets/maps.hpp"
#include "opeu/assets/pack-error.hpp"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::map_tables {

inline constexpr auto MAP_TABLE{ std::string_view{ "tables/maps.json" } };
inline constexpr auto BATTLE_FIELD_TABLE{ std::string_view{ "tables/battle-fields.json" } };

using CellGrids = std::array<CellGrid, MAP_COUNT>;

// The three cell layers of the six maps, as DecodeCellGrids reads them from MAPINF.DAT, MAPPNO.DAT and HEXDISP.DAT.
struct MapLayers {
  CellGrids cells;         // MAPINF: terrain class, bits 0..3; side 0 holds the cell, bit 6; side 1, bit 7
  CellGrids tiles;         // MAPPNO: the tile drawn for the cell
  CellGrids battle_fields; // HEXDISP: the cell's battle field, 1-based within its map

  auto operator==(MapLayers const&) const -> bool = default;
};

// The table file of the map layers (MAPINF's bits 4 and 5, unused, must be clear).
[[nodiscard]] auto MapTableText(MapLayers const& layers) -> std::string;

[[nodiscard]] auto ReadMapTable(Bytes bytes) -> Packed<MapLayers>;

[[nodiscard]] auto BattleFieldTableText(std::span<BattleField const> fields) -> std::string;

[[nodiscard]] auto ReadBattleFieldTable(Bytes bytes) -> Packed<std::vector<BattleField>>;

}

namespace opeu::assets {
using detail::map_tables::BATTLE_FIELD_TABLE;
using detail::map_tables::BattleFieldTableText;
using detail::map_tables::CellGrids;
using detail::map_tables::MAP_TABLE;
using detail::map_tables::MapLayers;
using detail::map_tables::MapTableText;
using detail::map_tables::ReadBattleFieldTable;
using detail::map_tables::ReadMapTable;
}
