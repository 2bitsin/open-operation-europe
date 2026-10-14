#include "opeu/assets/manifest.hpp"

#include "opeu/contract.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <format>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::manifest {

namespace {

using Json = nlohmann::json;

auto Optional(Json const& entry, std::string_view const key) -> std::optional<std::string> {
  if (!entry.contains(key)) return std::nullopt;
  return entry.at(key).get<std::string>();
}

auto Text(Json const& entry, std::string_view const key) -> std::string {
  return Optional(entry, key).value_or(std::string{});
}

auto ToAsset(Json const& entry, std::uint32_t const pack_scale) -> PackedAsset {
  return PackedAsset{
    .id          = entry.at("id").get<std::string>(),
    .asset_class = entry.at("class").get<std::string>(),
    .role        = entry.at("role").get<std::string>(),
    .width       = entry.at("width").get<std::size_t>(),
    .height      = entry.at("height").get<std::size_t>(),
    .scale       = entry.value("scale", pack_scale),
    .palette     = entry.at("palette").get<std::string>(),
    .palettes    = entry.at("palettes").get<std::vector<std::string>>(),
    .mask        = entry.at("mask").get<bool>(),
    .colours     = entry.value("colours", std::vector<std::uint8_t>{}),
    .image       = entry.at("image").get<std::string>(),
    .recolour    = Optional(entry, "recolour"),
    .indexed     = Optional(entry, "indexed"),
    .animation   = Text(entry, "animation"),
    .frame       = entry.value("frame", 0U),
    .variant_of  = Text(entry, "variant_of"),
    .variant     = Text(entry, "variant"),
    .use         = Text(entry, "use"),
    .description = Text(entry, "description"),
  };
}

auto ToColours(Json const& rows) -> PaletteColours {
  auto colours{ PaletteColours{} };
  std::ranges::transform(rows.get<std::vector<std::array<std::uint8_t, 3>>>() | std::views::take(colours.size()), colours.begin(),
                         [](std::array<std::uint8_t, 3> const& rgb) -> Colour { return Colour{ .r = std::get<0>(rgb), .g = std::get<1>(rgb), .b = std::get<2>(rgb) }; });
  return colours;
}

auto ToFont(Json const& font) -> Font {
  auto const cell{ font.at("cell").get<std::array<std::size_t, 2>>() };
  return Font{ .cell_width = std::get<0>(cell), .cell_height = std::get<1>(cell), .first_code = font.value("first", 0U), .glyphs = font.at("glyphs").get<std::vector<std::string>>() };
}

auto ToIds(Json const& ids) -> std::vector<std::string> {
  return ids.get<std::vector<std::string>>();
}

auto ToAssets(Json const& entries, std::uint32_t const pack_scale) -> Manifest::Assets {
  return entries | std::views::transform([pack_scale](Json const& entry) -> std::pair<std::string, PackedAsset> { return { entry.at("id").get<std::string>(), ToAsset(entry, pack_scale) }; })
         | std::ranges::to<Manifest::Assets>();
}

// The members of a JSON object, each value converted.
template <class _Value, class _Convert>
auto ByName(Json const& object, _Convert convert) -> std::map<std::string, _Value, std::less<>> {
  return object.items() | std::views::transform([&convert](auto const& item) -> std::pair<std::string, _Value> { return { item.key(), convert(item.value()) }; })
         | std::ranges::to<std::map<std::string, _Value, std::less<>>>();
}

auto ToContents(Json const& pack) -> Manifest::Contents {
  auto const scale{ pack.at("scale").get<std::uint32_t>() };
  return Manifest::Contents{
    .name       = pack.at("name").get<std::string>(),
    .scale      = scale,
    .assets     = ToAssets(pack.at("assets"), scale),
    .palettes   = ByName<PaletteColours>(pack.at("palettes"), ToColours),
    .animations = ByName<std::vector<std::string>>(pack.value("animations", Json::object()), ToIds),
    .fonts      = ByName<Font>(pack.value("fonts", Json::object()), ToFont),
  };
}

template <class _Map>
auto Lookup(_Map const& map, std::string_view const key) -> std::optional<typename _Map::mapped_type> {
  if (auto const found{ map.find(key) }; found != map.end()) return found->second;
  return std::nullopt;
}

}

Manifest::Manifest(Contents contents) : _contents{ std::move(contents) } {
  Expects(_contents.scale > 0, "a pack's images are at least the game's size");
}

auto Manifest::Parse(Bytes const json) -> Packed<Manifest> {
  try {
    auto const pack = Json::parse(AsText(json)); // not braces: a braced json is an array holding it
    if (pack.at("format").get<std::string>() != PACK_FORMAT) return Fail(PackFault::BAD_MANIFEST, std::format("not {}", PACK_FORMAT));
    auto contents{ ToContents(pack) };
    if (contents.scale == 0) return Fail(PackFault::BAD_MANIFEST, "scale 0");
    if (std::ranges::any_of(contents.assets | std::views::values, [](PackedAsset const& asset) { return asset.scale == 0; })) return Fail(PackFault::BAD_MANIFEST, "an entry of scale 0");
    return Manifest{ std::move(contents) };
  } catch (Json::exception const& error) {
    return Fail(PackFault::BAD_MANIFEST, error.what());
  }
}

auto Manifest::Find(std::string_view const id) const -> std::optional<PackedAsset> {
  return Lookup(_contents.assets, id);
}

auto Manifest::Ids() const -> std::vector<std::string> {
  return _contents.assets | std::views::keys | std::ranges::to<std::vector>();
}

auto Manifest::Palette(std::string_view const name) const -> std::optional<PaletteColours> {
  return Lookup(_contents.palettes, name);
}

auto Manifest::Frames(std::string_view const animation) const -> std::optional<std::vector<std::string>> {
  return Lookup(_contents.animations, animation);
}

auto Manifest::FontNamed(std::string_view const name) const -> std::optional<Font> {
  return Lookup(_contents.fonts, name);
}

auto Manifest::FontNames() const -> std::vector<std::string> {
  return _contents.fonts | std::views::keys | std::ranges::to<std::vector>();
}

}
