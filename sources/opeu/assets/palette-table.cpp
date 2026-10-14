#include "opeu/assets/palette-table.hpp"

#include "opeu/assets/table-json.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::palette_table {

namespace {

using namespace table_json;

constexpr auto COLOURS{ 16uz };
constexpr auto LEVEL{ 0x0FU };

// One colour's three levels, in the order the game stores them.
auto ColourJson(std::span<std::uint8_t const, 3> const levels) -> Json {
  auto colour     = Json::object();
  colour["blue"]  = levels[0];
  colour["red"]   = levels[1];
  colour["green"] = levels[2];
  return colour;
}

auto PaletteJson(NamedPalette const& palette) -> Json {
  auto record    = Json::object();
  record["name"] = palette.name;
  record["use"]  = palette.use;
  auto const levels{ std::span{ palette.levels } };
  record["colours"] = Json(Indices(COLOURS) | std::views::transform([levels](std::size_t const n) { return ColourJson(levels.subspan(3 * n).first<3>()); }) | std::ranges::to<std::vector<Json>>());
  return record;
}

auto LevelsOf(Json const& record) -> Levels {
  auto       levels{ Levels{} };
  auto const values{ Sized(record, "colours", COLOURS)
                     | std::views::transform([](Json const& colour) { return std::array{ Number(colour, "blue", LEVEL), Number(colour, "red", LEVEL), Number(colour, "green", LEVEL) }; })
                     | std::views::join | std::ranges::to<std::vector<std::uint32_t>>() };
  std::ranges::transform(values, levels.begin(), [](std::uint32_t const level) { return static_cast<std::uint8_t>(level); });
  return levels;
}

auto PaletteOf(Json const& record) -> PaletteRecord {
  return PaletteRecord{ .name = record.at("name").get<std::string>(), .levels = LevelsOf(record), .use = record.at("use").get<std::string>() };
}

}

auto PaletteTableText(std::span<NamedPalette const> const palettes) -> std::string {
  return TableText("palettes", Json(palettes | std::views::transform(PaletteJson) | std::ranges::to<std::vector<Json>>()));
}

auto ReadPaletteTable(Bytes const bytes) -> Packed<std::vector<PaletteRecord>> {
  return ReadTable(bytes, "palettes", [](Json const& palettes) {
    if (!palettes.is_array()) throw std::invalid_argument{ "palettes is not an array" };
    return palettes | std::views::transform(PaletteOf) | std::ranges::to<std::vector>();
  });
}

}
