#include "opeu/render/atlas-text-renderer.hpp"

#include <algorithm>
#include <format>
#include <ranges>
#include <utility>

namespace opeu::render::detail::atlas_text_renderer {
namespace {

[[nodiscard]] auto CellAt(TextRun const& run, std::size_t const index, Size const cell) -> Rect {
  return At(Point{ .x = run.at.x + (static_cast<int>(index) * cell.width), .y = run.at.y }, cell);
}

[[nodiscard]] auto RunArea(TextRun const& run, Size const cell) -> Rect {
  return At(run.at, Size{ .width = static_cast<int>(run.glyphs.size()) * cell.width, .height = cell.height });
}

}

AtlasTextRenderer::AtlasTextRenderer(Atlases atlases) : _atlases{ std::move(atlases) } { }

auto AtlasTextRenderer::Build(SDL_Renderer& renderer, assets::AssetPack const& pack) -> Rendered<AtlasTextRenderer> {
  auto atlases{ Atlases{} };
  for (auto const& name : pack.FontNames()) {
    auto atlas{ FontAtlas::Build(renderer, pack, name) };
    if (!atlas) return std::unexpected{ atlas.error() };
    atlases.emplace(name, *std::move(atlas));
  }
  return AtlasTextRenderer{ std::move(atlases) };
}

auto AtlasTextRenderer::Font(std::string_view const name) const -> Rendered<std::reference_wrapper<FontAtlas const>> {
  if (auto const found{ _atlases.find(name) }; found != _atlases.end()) return std::cref(found->second);
  return Failed(std::format("no font {}", name));
}

auto AtlasTextRenderer::Draw(Display& display, TextRun const& run) const -> Rendered<void> {
  auto const font{ Font(run.font) };
  if (!font) return std::unexpected{ font.error() };
  auto const& atlas{ font->get() };
  if (!std::ranges::all_of(run.glyphs, [&atlas](GlyphIndex const glyph) { return atlas.Area(glyph).has_value(); })) return Failed(std::format("a glyph past the end of {}", run.font));
  if (run.paper) display.Fill(RunArea(run, atlas.Metrics().cell), *run.paper);
  for (auto const [index, glyph] : std::views::zip(std::views::iota(0uz), run.glyphs)) {
    auto const blit{
      Blit{ .destination = CellAt(run, index, atlas.Metrics().cell), .source = atlas.Area(glyph), .modulation = run.colour }
    };
    display.Draw(atlas.Atlas(), blit);
  }
  return {};
}

auto AtlasTextRenderer::Measure(TextRun const& run) const -> Rendered<Size> {
  auto const font{ Font(run.font) };
  if (!font) return std::unexpected{ font.error() };
  return Dimensions(RunArea(run, font->get().Metrics().cell));
}

}
