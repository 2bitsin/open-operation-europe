#pragma once
// Draws a Scene on the Display: sprites from the texture cache, fills and lines as the renderer's own, text through
// the text renderer, in the scene's order; the scene's tint is applied when the frame is shown.

#include "opeu/render/display.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/scene.hpp"
#include "opeu/render/text-renderer.hpp"
#include "opeu/render/texture-cache.hpp"

#include <functional>

namespace opeu::render::detail::scene_renderer {

class SceneRenderer {
public:
  SceneRenderer(Display& display, TextureCache& textures, TextRenderer const& text);

  // Begin, the scene's commands, Present with its tint.
  auto Show(Scene const& scene) -> Rendered<void>;
  // The scene's commands on the current target (the frame, or a layer); the tint is the caller's.
  auto Compose(Scene const& scene) -> Rendered<void>;

private:
  auto Draw(Sprite const& sprite) -> Rendered<void>;
  auto Draw(Fill const& fill) -> Rendered<void>;
  auto Draw(Line const& line) -> Rendered<void>;
  auto Draw(Text const& text) -> Rendered<void>;

  [[nodiscard]] auto AssetOf(Sprite const& sprite) const -> Rendered<std::string>;

  std::reference_wrapper<Display>            _display;
  std::reference_wrapper<TextureCache>       _textures;
  std::reference_wrapper<TextRenderer const> _text;
};

}

namespace opeu::render {
using detail::scene_renderer::SceneRenderer;
}
