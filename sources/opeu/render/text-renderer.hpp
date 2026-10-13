#pragma once
// Text on the Display: a run of glyphs in a font, a pen colour and a position. The interface is the extension point
// for a true-type renderer (SDL_ttf through conan, the original metrics: fixed cells, pen colours); the pack's fonts
// are drawn by AtlasTextRenderer.

#include "opeu/render/display.hpp"
#include "opeu/render/font-atlas.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/render-error.hpp"

#include <string>
#include <vector>

namespace opeu::render::detail::text_renderer {

struct TextRun {
  std::string             font{};          // a font of the pack: "game-glyph", "ending-glyph"
  std::vector<GlyphIndex> glyphs{};        // in the font's code order
  Rgba                    colour{ WHITE }; // the pen
  Point                   at{};            // the first cell's top left corner

  auto operator==(TextRun const&) const -> bool = default;
};

class TextRenderer {
public:
  virtual ~TextRenderer() = default;

  virtual auto               Draw(Display& display, TextRun const& run) const -> Rendered<void> = 0;
  [[nodiscard]] virtual auto Measure(TextRun const& run) const -> Rendered<Size>                = 0;

protected:
  TextRenderer()                                           = default;
  TextRenderer(TextRenderer const&)                        = default;
  TextRenderer(TextRenderer&&) noexcept                    = default;
  auto operator=(TextRenderer const&) -> TextRenderer&     = default;
  auto operator=(TextRenderer&&) noexcept -> TextRenderer& = default;
};

}

namespace opeu::render {
using detail::text_renderer::TextRenderer;
using detail::text_renderer::TextRun;
}
