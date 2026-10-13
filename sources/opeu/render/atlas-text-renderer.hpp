#pragma once
// The pack's fonts as atlases: a glyph is a blit from its font's atlas into a fixed cell, in the pen colour by colour
// modulation, the pen advancing a cell per glyph; a run with paper fills its cells first.

#include "opeu/assets/pack.hpp"
#include "opeu/render/font-atlas.hpp"
#include "opeu/render/text-renderer.hpp"

#include <SDL3/SDL.h>

#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace opeu::render::detail::atlas_text_renderer {

class AtlasTextRenderer final : public TextRenderer {
public:
  // An atlas for every font the pack lists.
  [[nodiscard]] static auto Build(SDL_Renderer& renderer, assets::AssetPack const& pack) -> Rendered<AtlasTextRenderer>;

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
}
