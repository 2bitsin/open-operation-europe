#include "opeu/render/texture-cache.hpp"

#include <format>
#include <utility>

namespace opeu::render::detail::texture_cache {
namespace {

[[nodiscard]] auto GameSize(assets::PackedAsset const& entry) -> Size {
  return Size{ .width = static_cast<int>(entry.width), .height = static_cast<int>(entry.height) };
}

}

TextureCache::TextureCache(SDL_Renderer& renderer, assets::AssetPack const& pack, std::size_t const capacity) : _renderer{ renderer }, _pack{ pack }, _baked{ capacity } { }

auto TextureCache::Fetch(TextureKey const& key) -> Rendered<SharedBaked> {
  if (auto found{ _baked.Find(key) }) return *std::move(found);
  auto baked{ Bake(key) };
  if (!baked) return std::unexpected{ baked.error() };
  auto shared{ std::make_shared<Baked const>(*std::move(baked)) };
  _baked.Remember(key, shared);
  return shared;
}

auto TextureCache::Bake(TextureKey const& key) const -> Rendered<Baked> {
  auto const asset{ _pack.get().Load(key.asset) };
  if (!asset) return PackFailed(asset.error());
  auto const pixels{ Pixels(*asset, key) };
  if (!pixels) return std::unexpected{ pixels.error() };
  auto texture{ Texture::Upload(_renderer.get(), *pixels) };
  if (!texture) return std::unexpected{ texture.error() };
  return Baked{ .texture = *std::move(texture), .size = GameSize(asset->entry) };
}

auto TextureCache::Pixels(assets::LoadedAsset const& asset, TextureKey const& key) const -> Rendered<assets::Texture> {
  auto const colours{ ColoursOf(key) };
  if (!colours) return std::unexpected{ colours.error() };
  auto const own{ _pack.get().Palette(asset.entry.palette) };
  auto const background{ own.transform([](assets::PaletteColours const& palette) { return palette.front(); }) };
  if (!*colours) return KeyedImage(asset.texture, key.keying, background);
  return render::Bake(asset, Painting{ .colours = **colours, .keying = key.keying, .background = background });
}

// The colours a key bakes in: its own, its named palette's, or none (the asset's own image).
auto TextureCache::ColoursOf(TextureKey const& key) const -> Rendered<std::optional<assets::PaletteColours>> {
  if (key.colours) return key.colours;
  if (key.palette.empty()) return std::optional<assets::PaletteColours>{};
  auto const colours{ _pack.get().Palette(key.palette) };
  if (!colours) return Failed(std::format("no palette {}", key.palette));
  return colours;
}

}
