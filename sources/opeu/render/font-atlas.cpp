#include "opeu/render/font-atlas.hpp"

#include "opeu/render/baking.hpp"
#include "opeu/render/target.hpp"

#include <format>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace opeu::render::detail::font_atlas {
namespace {

// The glyph images in font order, inked white.
[[nodiscard]] auto InkedGlyphs(assets::AssetPack const& pack, assets::Font const& font) -> Rendered<std::vector<assets::Texture>> {
  auto glyphs{ std::vector<assets::Texture>{} };
  glyphs.reserve(font.glyphs.size());
  for (auto const& id : font.glyphs) {
    auto const glyph{ pack.Load(id) };
    if (!glyph) return PackFailed(glyph.error());
    glyphs.push_back(Bake(*glyph, INK));
  }
  if (glyphs.empty()) return Failed("a font has glyphs");
  return glyphs;
}

[[nodiscard]] auto Scaled(Size const size, std::uint32_t const scale) -> Size {
  return Size{ .width = size.width * static_cast<int>(scale), .height = size.height * static_cast<int>(scale) };
}

[[nodiscard]] auto MetricsOf(assets::Font const& font, std::uint32_t const scale) -> FontMetrics {
  auto const cell{
    Size{ .width = static_cast<int>(font.cell_width), .height = static_cast<int>(font.cell_height) }
  };
  return FontMetrics{ .cell = cell, .slot = Scaled(cell, scale), .count = font.glyphs.size(), .first_code = font.first_code };
}

[[nodiscard]] auto SlotArea(std::size_t const index, Size const slot) -> Rect {
  auto const column{ static_cast<int>(index % FontAtlas::COLUMNS) };
  auto const row{ static_cast<int>(index / FontAtlas::COLUMNS) };
  return At(Point{ .x = column * slot.width, .y = row * slot.height }, slot);
}

[[nodiscard]] auto AtlasSize(std::size_t const count, Size const slot) -> Size {
  auto const rows{ static_cast<int>((count + FontAtlas::COLUMNS - 1) / FontAtlas::COLUMNS) };
  return Size{ .width = static_cast<int>(FontAtlas::COLUMNS) * slot.width, .height = rows * slot.height };
}

// The glyphs an atlas holds, each in a slot of one size.
struct Slotted {
  std::span<assets::Texture const> glyphs;
  Size                             slot;
};

// Blits every glyph into its slot of the atlas, scaled to fill it.
[[nodiscard]] auto Compose(SDL_Renderer& renderer, Texture& atlas, Slotted const slotted) -> Rendered<void> {
  auto const glyphs{ slotted.glyphs };
  auto const slot{ slotted.slot };
  auto const into{
    Target{ renderer, atlas }
  };
  for (auto const [index, image] : std::views::zip(std::views::iota(0uz), glyphs)) {
    auto const glyph{ Texture::Upload(renderer, image) };
    if (!glyph) return std::unexpected{ glyph.error() };
    SDL_SetTextureBlendMode(&glyph->Handle(), SDL_BLENDMODE_NONE);
    auto const area{ SlotArea(index, slot) };
    auto const destination{
      SDL_FRect{ .x = static_cast<float>(area.x), .y = static_cast<float>(area.y), .w = static_cast<float>(area.width), .h = static_cast<float>(area.height) }
    };
    SDL_RenderTexture(&renderer, &glyph->Handle(), nullptr, &destination);
  }
  return {};
}

}

FontAtlas::FontAtlas(Texture atlas, FontMetrics const metrics) : _atlas{ std::move(atlas) }, _metrics{ metrics } { }

auto FontAtlas::Build(SDL_Renderer& renderer, assets::AssetPack const& pack, std::string_view const name) -> Rendered<FontAtlas> {
  auto const font{ pack.FontNamed(name) };
  if (!font) return Failed(std::format("no font {}", name));
  auto const glyphs{ InkedGlyphs(pack, *font) };
  if (!glyphs) return Failed(std::format("font {}: {}", name, glyphs.error().detail));
  auto const metrics{ MetricsOf(*font, pack.Scale()) };
  auto       atlas{ Texture::Target(renderer, AtlasSize(metrics.count, metrics.slot)) };
  if (!atlas) return std::unexpected{ atlas.error() };
  if (auto const composed{ Compose(renderer, *atlas, Slotted{ .glyphs = *glyphs, .slot = metrics.slot }) }; !composed) return std::unexpected{ composed.error() };
  return FontAtlas{ *std::move(atlas), metrics };
}

auto FontAtlas::Area(GlyphIndex const glyph) const -> std::optional<Rect> {
  if (glyph >= _metrics.count) return std::nullopt;
  return SlotArea(glyph, _metrics.slot);
}

}
