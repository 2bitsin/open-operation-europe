#include "opeu/assets/map-tables.hpp"

#include "opeu/assets/table-json.hpp"
#include "opeu/contract.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::map_tables {
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace {

using namespace table_json;

constexpr auto TERRAIN_CLASS{ 0x0FU };
constexpr auto SIDE_0_HOLDS{ 6U };
constexpr auto SIDE_1_HOLDS{ 7U };
constexpr auto TERRAIN_CODE{ 0x3FU };
constexpr auto VARIANT{ 6U };

// The cells with `extract` applied to each.
template <class _Cells, class _Extract>
auto Each(_Cells const& cells, _Extract extract) {
  return cells | std::views::transform([extract](std::uint8_t const cell) { return static_cast<std::uint8_t>(extract(cell)); });
}

auto Bit(std::uint32_t const bit) {
  return [bit](std::uint8_t const cell) { return cell >> bit & 1U; };
}

auto MapJson(MapLayers const& layers, std::size_t const map) -> Json {
  auto const& cells{ layers.cells.at(map) };
  Expects(std::ranges::none_of(cells, [](std::uint8_t const cell) { return (cell & 0x30U) != 0; }), "MAPINF's bits 4 and 5 are unused");
  auto record             = Json::object();
  record["terrain_class"] = Rows(Each(cells, [](std::uint8_t const cell) { return cell & TERRAIN_CLASS; }), MAP_SIDE);
  record["side_0_holds"]  = Rows(Each(cells, Bit(SIDE_0_HOLDS)), MAP_SIDE);
  record["side_1_holds"]  = Rows(Each(cells, Bit(SIDE_1_HOLDS)), MAP_SIDE);
  record["tile"]          = Rows(layers.tiles.at(map), MAP_SIDE);
  record["battle_field"]  = Rows(layers.battle_fields.at(map), MAP_SIDE);
  return record;
}

auto CellsOf(Json const& record) -> CellGrid {
  auto const terrain{ Cells<MAP_SIDE>(record, "terrain_class", TERRAIN_CLASS) };
  auto const side_0{ Cells<MAP_SIDE>(record, "side_0_holds", 1) };
  auto const side_1{ Cells<MAP_SIDE>(record, "side_1_holds", 1) };
  auto       cells{ CellGrid{} };
  std::ranges::transform(std::views::zip(terrain, side_0, side_1), cells.begin(), [](auto const parts) {
    auto const [terrain_class, holds_0, holds_1]{ parts };
    return static_cast<std::uint8_t>(terrain_class | holds_0 << SIDE_0_HOLDS | holds_1 << SIDE_1_HOLDS);
  });
  return cells;
}

auto LayersOf(Json const& listed) -> MapLayers {
  auto        layers{ MapLayers{} };
  auto const& maps{ listed };
  CheckCount(maps, "maps", MAP_COUNT);
  std::ranges::transform(maps, layers.cells.begin(), CellsOf);
  std::ranges::transform(maps, layers.tiles.begin(), [](Json const& record) { return Cells<MAP_SIDE>(record, "tile", 0xFF); });
  std::ranges::transform(maps, layers.battle_fields.begin(), [](Json const& record) { return Cells<MAP_SIDE>(record, "battle_field", 0xFF); });
  return layers;
}

auto FieldJson(BattleField const& field) -> Json {
  auto record            = Json::object();
  record["terrain_code"] = Rows(Each(field, [](std::uint8_t const cell) { return cell & TERRAIN_CODE; }), FIELD_SIDE);
  record["variant"]      = Rows(Each(field, [](std::uint8_t const cell) { return cell >> VARIANT; }), FIELD_SIDE);
  return record;
}

auto FieldOf(Json const& record) -> BattleField {
  auto const terrain{ Cells<FIELD_SIDE>(record, "terrain_code", TERRAIN_CODE) };
  auto const variant{ Cells<FIELD_SIDE>(record, "variant", 3) };
  auto       field{ BattleField{} };
  std::ranges::transform(terrain, variant, field.begin(), [](std::uint8_t const code, std::uint8_t const bits) { return static_cast<std::uint8_t>(code | bits << VARIANT); });
  return field;
}

}

auto MapTableText(MapLayers const& layers) -> std::string {
  return TableText("maps", Json(Indices(MAP_COUNT) | std::views::transform([&layers](std::size_t const map) { return MapJson(layers, map); }) | std::ranges::to<std::vector<Json>>()));
}

auto ReadMapTable(Bytes const bytes) -> Packed<MapLayers> {
  return ReadTable(bytes, "maps", LayersOf);
}

auto BattleFieldTableText(std::span<BattleField const> const fields) -> std::string {
  return TableText("battle_fields", Json(fields | std::views::transform(FieldJson) | std::ranges::to<std::vector<Json>>()));
}

auto ReadBattleFieldTable(Bytes const bytes) -> Packed<std::vector<BattleField>> {
  return ReadTable(bytes, "battle_fields", [](Json const& fields) {
    CheckCount(fields, "battle_fields", FIELD_COUNT);
    return fields | std::views::transform(FieldOf) | std::ranges::to<std::vector>();
  });
}

}
