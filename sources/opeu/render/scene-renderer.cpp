#include "opeu/render/scene-renderer.hpp"

#include <variant>

namespace opeu::render::detail::scene_renderer {

SceneRenderer::SceneRenderer(Display& display, TextureCache& textures, TextRenderer const& text) : _display{ display }, _textures{ textures }, _text{ text } { }

auto SceneRenderer::Show(Scene const& scene) -> Rendered<void> {
  _display.get().Begin();
  auto drawn{ Compose(scene) };
  _display.get().Present(scene.Tinted());
  return drawn;
}

auto SceneRenderer::Compose(Scene const& scene) -> Rendered<void> {
  for (auto const& command : scene.Commands()) {
    if (auto drawn{ std::visit([this](auto const& operation) { return Draw(operation); }, command) }; !drawn) return drawn;
  }
  return {};
}

auto SceneRenderer::AssetOf(Sprite const& sprite) const -> Rendered<std::string> {
  if (!sprite.frame) return sprite.asset;
  return _textures.get().Source().Frame(sprite.asset, *sprite.frame);
}

auto SceneRenderer::Draw(Sprite const& sprite) -> Rendered<void> {
  auto const asset{ AssetOf(sprite) };
  if (!asset) return std::unexpected{ asset.error() };
  auto const baked{ _textures.get().Fetch(TextureKey{ .asset = *asset, .palette = sprite.palette }) };
  if (!baked) return std::unexpected{ baked.error() };
  auto const destination{ At(sprite.at, sprite.size.value_or((*baked)->size)) };
  _display.get().Draw((*baked)->texture, Blit{ .destination = destination, .flip = sprite.flip, .modulation = sprite.modulation });
  return {};
}

auto SceneRenderer::Draw(Fill const& fill) -> Rendered<void> {
  _display.get().Fill(fill.area, fill.colour);
  return {};
}

auto SceneRenderer::Draw(Line const& line) -> Rendered<void> {
  _display.get().Line(line.from, line.to, line.colour);
  return {};
}

auto SceneRenderer::Draw(Text const& text) -> Rendered<void> {
  return _text.get().Draw(_display.get(), text);
}

}
