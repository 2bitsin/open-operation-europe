#pragma once
// One texture holding every glyph of a pack's font, built at load time by blitting the pack's per-glyph images into
// a grid, so a line of text is one texture's blits and a remaster can still replace single glyphs.

#include "opeu/render/geometry.hpp"
#include "opeu/render/pack-source.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/texture.hpp"

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace opeu::render::detail::font_atlas {

// A glyph's number in its font: its place in the manifest's list, the font's code order.
using GlyphIndex = std::uint32_t;

struct FontMetrics {
  Size        cell{};  // in game pixels: every glyph is drawn in it, and the pen advances by its width
  Size        slot{};  // in atlas pixels: the cell times the pack's scale
  std::size_t count{}; // glyphs
};

class FontAtlas {
public:
  static constexpr auto COLUMNS{ std::size_t{ 32 } }; // glyphs per row of the atlas

  // The font `name` of the pack, its glyphs in white (colour modulation gives the pen colour).
  [[nodiscard]] static auto Build(SDL_Renderer& renderer, PackSource const& source, std::string_view name) -> Rendered<FontAtlas>;

  [[nodiscard]] auto Metrics() const noexcept -> FontMetrics const& {
    return _metrics;
  }
  // Where the glyph sits in the atlas, or nothing past the font's last glyph.
  [[nodiscard]] auto Area(GlyphIndex glyph) const -> std::optional<Rect>;
  [[nodiscard]] auto Atlas() const noexcept -> Texture const& {
    return _atlas;
  }

private:
  FontAtlas(Texture atlas, FontMetrics metrics);

  Texture     _atlas;
  FontMetrics _metrics;
};

}

namespace opeu::render {
using detail::font_atlas::FontAtlas;
using detail::font_atlas::FontMetrics;
using detail::font_atlas::GlyphIndex;
}
