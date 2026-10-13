#include "opeu/render/font-atlas.hpp"

#include "opeu/render/baking.hpp"
#include "opeu/render/target.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <utility>
#include <vector>

namespace opeu::render::detail::font_atlas {
namespace {

// The glyph images in font order, inked white; they must all be one size.
[[nodiscard]] auto InkedGlyphs(PackSource const& source, assets::Font const& font) -> Rendered<std::vector<assets::Texture>> {
  auto glyphs{ std::vector<assets::Texture>{} };
  glyphs.reserve(font.glyphs.size());
  for (auto const& id : font.glyphs) {
    auto const glyph{ source.Load(id) };
    if (!glyph) return std::unexpected{ glyph.error() };
    glyphs.push_back(Bake(*glyph, INK));
  }
  auto const same_size{ [&glyphs](assets::Texture const& glyph) { return glyph.Width() == glyphs.front().Width() && glyph.Height() == glyphs.front().Height(); } };
  if (glyphs.empty() || !std::ranges::all_of(glyphs, same_size)) return Failed("a font's glyphs are one non-empty size");
  return glyphs;
}

[[nodiscard]] auto SlotOf(assets::Texture const& glyph) -> Size {
  return Size{ .width = static_cast<int>(glyph.Width()), .height = static_cast<int>(glyph.Height()) };
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

// Blits every glyph into its slot of the atlas.
[[nodiscard]] auto Compose(SDL_Renderer& renderer, Texture& atlas, std::span<assets::Texture const> glyphs) -> Rendered<void> {
  auto const into{
    Target{ renderer, atlas }
  };
  for (auto const [index, image] : std::views::zip(std::views::iota(0uz), glyphs)) {
    auto const glyph{ Texture::Upload(renderer, image) };
    if (!glyph) return std::unexpected{ glyph.error() };
    SDL_SetTextureBlendMode(&glyph->Handle(), SDL_BLENDMODE_NONE);
    auto const area{ SlotArea(index, glyph->Dimensions()) };
    auto const destination{
      SDL_FRect{ .x = static_cast<float>(area.x), .y = static_cast<float>(area.y), .w = static_cast<float>(area.width), .h = static_cast<float>(area.height) }
    };
    SDL_RenderTexture(&renderer, &glyph->Handle(), nullptr, &destination);
  }
  return {};
}

}

FontAtlas::FontAtlas(Texture atlas, FontMetrics const metrics) : _atlas{ std::move(atlas) }, _metrics{ metrics } { }

auto FontAtlas::Build(SDL_Renderer& renderer, PackSource const& source, std::string_view const name) -> Rendered<FontAtlas> {
  auto const font{ source.Font(name) };
  if (!font) return std::unexpected{ font.error() };
  auto const glyphs{ InkedGlyphs(source, *font) };
  if (!glyphs) return Failed(std::format("font {}: {}", name, glyphs.error().detail));
  auto const metrics{
    FontMetrics{ .cell = Size{ .width = static_cast<int>(font->cell_width), .height = static_cast<int>(font->cell_height) }, .slot = SlotOf(glyphs->front()), .count = glyphs->size() }
  };
  auto atlas{ Texture::Target(renderer, AtlasSize(metrics.count, metrics.slot)) };
  if (!atlas) return std::unexpected{ atlas.error() };
  if (auto const composed{ Compose(renderer, *atlas, *glyphs) }; !composed) return std::unexpected{ composed.error() };
  return FontAtlas{ *std::move(atlas), metrics };
}

auto FontAtlas::Area(GlyphIndex const glyph) const -> std::optional<Rect> {
  if (glyph >= _metrics.count) return std::nullopt;
  return SlotArea(glyph, _metrics.slot);
}

}
