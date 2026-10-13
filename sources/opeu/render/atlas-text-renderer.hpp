#pragma once
// The pack's fonts as atlases: a glyph is a blit from its font's atlas into a fixed cell, in the pen colour by colour
// modulation, the pen advancing a cell per glyph.

#include "opeu/render/font-atlas.hpp"
#include "opeu/render/pack-source.hpp"
#include "opeu/render/text-renderer.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>

namespace opeu::render::detail::atlas_text_renderer {

// The classic pack's fonts (the manifest's `fonts`).
inline constexpr auto GAME_FONT{ std::string_view{ "game-glyph" } };
inline constexpr auto ENDING_FONT{ std::string_view{ "ending-glyph" } };
inline constexpr auto CLASSIC_FONTS{
  std::array{ GAME_FONT, ENDING_FONT }
};

class AtlasTextRenderer final : public TextRenderer {
public:
  [[nodiscard]] static auto Build(SDL_Renderer& renderer, PackSource const& source, std::span<std::string_view const> fonts) -> Rendered<AtlasTextRenderer>;

  auto               Draw(Display& display, TextRun const& run) const -> Rendered<void> override;
  [[nodiscard]] auto Measure(TextRun const& run) const -> Rendered<Size> override;
  [[nodiscard]] auto Font(std::string_view name) const -> Rendered<std::reference_wrapper<FontAtlas const>>;

private:
  using Atlases = std::map<std::string, FontAtlas, std::less<>>;

  explicit AtlasTextRenderer(Atlases atlases);

  Atlases _atlases;
};

}

namespace opeu::render {
using detail::atlas_text_renderer::AtlasTextRenderer;
using detail::atlas_text_renderer::CLASSIC_FONTS;
using detail::atlas_text_renderer::ENDING_FONT;
using detail::atlas_text_renderer::GAME_FONT;
}
