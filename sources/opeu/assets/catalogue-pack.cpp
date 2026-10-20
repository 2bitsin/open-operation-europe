#include "opeu/assets/catalogue-pack.hpp"

#include "opeu/assets/palette.hpp"
#include "opeu/assets/texture.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::catalogue_pack {
using namespace std::string_literals;
using namespace std::string_view_literals;
namespace {

constexpr auto HOURS{
  std::array{ std::string_view{ "main/dawn" }, std::string_view{ "main/day" }, std::string_view{ "main/dusk" }, std::string_view{ "main/night" } }
};

// Why the game shows the asset in colours other than its image's: the hour's palette (colours 8..15), the pen it
// draws a one-bit shape with, or the opening's palette animation.
auto RecolourOf(Asset const& asset) -> std::optional<std::string> {
  if (asset.hour_coloured) return "hour";
  if (asset.layout == Layout::BITMAP || asset.layout == Layout::RUNS_BITMAP) return "ink";
  if (asset.palette.starts_with("open/")) return "scene";
  return std::nullopt;
}

// The palette indices the image draws, ascending.
auto ColoursDrawn(Image const& image) -> std::vector<std::uint8_t> {
  auto drawn{ Indices(image.Pixels().size()) | std::views::filter([&image](std::size_t const n) { return image.Opaque().empty() || image.Opaque()[n] != 0; })
              | std::views::transform([&image](std::size_t const n) { return image.Pixels()[n]; }) | std::ranges::to<std::vector>() };
  std::ranges::sort(drawn);
  auto const repeated{ std::ranges::unique(drawn) };
  drawn.erase(repeated.begin(), repeated.end());
  return drawn;
}

// The image as its index map reads back: index 0 where the background shows, a mask only when some pixel lets it.
auto IndexMap(Image const& image) -> Image {
  auto const count{ image.Pixels().size() };
  auto const drawn{ [&image](std::size_t const n) { return image.Opaque().empty() || image.Opaque()[n] != 0; } };
  auto       pixels{ Indices(count) | std::views::transform([&](std::size_t const n) { return drawn(n) ? image.Pixels()[n] : std::uint8_t{ 0 }; }) | std::ranges::to<std::vector>() };
  if (std::ranges::all_of(Indices(count), drawn)) return Image{ image.Shape(), std::move(pixels) };
  auto opaque{ Indices(count) | std::views::transform([&](std::size_t const n) { return static_cast<std::uint8_t>(drawn(n)); }) | std::ranges::to<std::vector>() };
  return Image{ image.Shape(), std::move(pixels), std::move(opaque) };
}

auto Palettes(Asset const& asset) -> std::vector<std::string> {
  if (asset.hour_coloured) return HOURS | std::ranges::to<std::vector<std::string>>();
  return { std::string{ asset.palette } };
}

auto Entry(std::string id, Asset const& asset, Image const& image) -> PackedAsset {
  auto const recolour{ RecolourOf(asset) };
  return PackedAsset{
    .id          = id,
    .asset_class = std::string{ asset.group },
    .role        = std::string{ Spelling(asset.role) },
    .width       = image.Width(),
    .height      = image.Height(),
    .scale       = 1,
    .palette     = std::string{ asset.palette },
    .palettes    = Palettes(asset),
    .mask        = image.HasMask(),
    .colours     = ColoursDrawn(image),
    .image       = std::format("{}/{}.png", asset.group, id),
    .recolour    = recolour,
    .indexed     = recolour.transform([&](std::string const&) { return std::format("{}/{}.index.png", asset.group, id); }),
    .animation   = asset.animation,
    .frame       = asset.frame,
    .variant_of  = {},
    .variant     = {},
    .use         = asset.use,
    .description = {},
  };
}

// A unit icon's side-0 variant: side 0's colours, never recoloured otherwise.
auto VariantEntry(std::string const& of, Asset const& asset, Image const& image) -> PackedAsset {
  auto entry{ Entry(std::format("{}/{}", of, SIDE_ZERO), asset, image) };
  entry.recolour.reset();
  entry.indexed.reset();
  entry.variant_of = of;
  entry.variant    = SIDE_ZERO;
  return entry;
}

// The number a glyph's id ends in: its code in its font.
auto CodeOf(std::string_view const id) -> std::uint32_t {
  auto const digits{ id.substr(id.rfind('/') + 1) | std::views::take_while([](char const c) { return c >= '0' && c <= '9'; }) };
  return std::ranges::fold_left(digits, std::uint32_t{}, [](std::uint32_t const code, char const digit) { return (code * 10) + static_cast<std::uint32_t>(digit - '0'); });
}

// A glyph's cell: the font's cell before scaling.
auto FontOf(Asset const& glyph) -> Font {
  return Font{ .cell_width = glyph.width != 0 ? glyph.width : glyph.width_bytes * 8uz, .cell_height = glyph.height, .first_code = CodeOf(glyph.id), .glyphs = {} };
}

}

CataloguePack::CataloguePack(Catalogue catalogue) : _catalogue{ std::move(catalogue) } {
  std::ranges::for_each(_catalogue.Assets(), [this](Asset const& asset) {
    _listed.emplace(asset.id, Listed{ .asset = asset, .variant = {} });
    if (asset.side_coloured) _listed.emplace(std::format("{}/{}", asset.id, SIDE_ZERO), Listed{ .asset = asset, .variant = std::string{ SIDE_ZERO } });
    if (!asset.animation.empty()) _animations[asset.animation].push_back(asset.id); // the catalogue lists frames in order
    if (asset.role != Role::GLYPH) return;
    auto const name{ asset.id.substr(0, asset.id.find('/')) };
    auto&      font{ _fonts.try_emplace(name, FontOf(asset)).first->second };
    font.glyphs.push_back(asset.id);
  });
}

auto CataloguePack::Draw(std::string_view const id) const -> Packed<Drawn> {
  auto const found{ _listed.find(id) };
  if (found == _listed.end()) return Fail(PackFault::UNKNOWN_ASSET, std::string{ id });
  auto const& [asset, variant]{ found->second };
  auto const image{ _catalogue.Decode(asset) };
  if (!image) return Fail(PackFault::BAD_IMAGE, std::format("{}: {}", id, Describe(image.error())));
  if (variant.empty()) return Drawn{ .entry = Entry(asset.id, asset, *image), .image = *image };
  auto const side_zero{ SideZeroColours(*image) };
  return Drawn{ .entry = VariantEntry(asset.id, asset, side_zero), .image = side_zero };
}

auto CataloguePack::Find(std::string_view const id) const -> std::optional<PackedAsset> {
  auto drawn{ Draw(id) };
  if (!drawn) return std::nullopt;
  return std::move(drawn->entry);
}

auto CataloguePack::Load(std::string_view const id) const -> Packed<LoadedAsset> {
  auto drawn{ Draw(id) };
  if (!drawn) return std::unexpected{ drawn.error() };
  auto const colours{ Palette(drawn->entry.palette) };
  if (!colours) return Fail(PackFault::BAD_MANIFEST, std::format("{}: no palette {}", id, drawn->entry.palette));
  auto texture{ ToTexture(drawn->image, *colours) };
  auto indexed{ drawn->entry.recolour ? std::optional{ IndexMap(drawn->image) } : std::nullopt };
  return LoadedAsset{ .entry = std::move(drawn->entry), .texture = std::move(texture), .indexed = std::move(indexed) };
}

auto CataloguePack::Palette(std::string_view const name) const -> std::optional<PaletteColours> {
  return FindPalette(name).transform([](NamedPalette const& palette) { return Colours(palette.levels); });
}

auto CataloguePack::Ids() const -> std::vector<std::string> {
  return _listed | std::views::keys | std::ranges::to<std::vector>();
}

auto CataloguePack::Frames(std::string_view const animation) const -> std::optional<std::vector<std::string>> {
  if (auto const found{ _animations.find(animation) }; found != _animations.end()) return found->second;
  return std::nullopt;
}

auto CataloguePack::FontNamed(std::string_view const name) const -> std::optional<Font> {
  if (auto const found{ _fonts.find(name) }; found != _fonts.end()) return found->second;
  return std::nullopt;
}

auto CataloguePack::FontNames() const -> std::vector<std::string> {
  return _fonts | std::views::keys | std::ranges::to<std::vector>();
}

auto CataloguePack::Scale() const -> std::uint32_t {
  return 1;
}

}
