#include "opeu/render/scene-renderer.hpp"

#include <algorithm>
#include <format>
#include <ranges>
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
  auto const frames{ _textures.get().Pack().Frames(sprite.asset) };
  if (!frames) return Failed(std::format("no animation {}", sprite.asset));
  if (*sprite.frame >= frames->size()) return Failed(std::format("{} has {} frames, not {}", sprite.asset, frames->size(), *sprite.frame + 1));
  return frames->at(*sprite.frame);
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

auto SceneRenderer::Draw(Tiles const& tiles) -> Rendered<void> {
  auto const columns{ static_cast<int>(std::max(tiles.columns, 1uz)) };
  for (auto const [index, asset] : std::views::zip(std::views::iota(0), tiles.assets)) {
    auto const at{
      Point{ .x = tiles.at.x + (index % columns * tiles.cell.width), .y = tiles.at.y + (index / columns * tiles.cell.height) }
    };
    if (auto const placed{ Place(TextureKey{ .asset = asset, .palette = tiles.palette }, at) }; !placed) return placed;
  }
  return {};
}

auto SceneRenderer::Place(TextureKey const& key, Point const at) -> Rendered<void> {
  auto const baked{ _textures.get().Fetch(key) };
  if (!baked) return std::unexpected{ baked.error() };
  _display.get().Draw((*baked)->texture, Blit{ .destination = At(at, (*baked)->size) });
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
