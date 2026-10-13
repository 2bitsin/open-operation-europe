#include "opeu/render/texture-cache.hpp"

#include "opeu/contract.hpp"
#include "opeu/render/baking.hpp"

#include <format>
#include <utility>

namespace opeu::render::detail::texture_cache {
namespace {

[[nodiscard]] auto GameSize(assets::PackedAsset const& entry) -> Size {
  return Size{ .width = static_cast<int>(entry.width), .height = static_cast<int>(entry.height) };
}

}

TextureCache::TextureCache(SDL_Renderer& renderer, assets::AssetPack const& pack, std::size_t const capacity) : _renderer{ renderer }, _pack{ pack }, _capacity{ capacity } {
  Expects(capacity > 0, "a texture cache holds at least one texture");
}

auto TextureCache::Fetch(TextureKey const& key) -> Rendered<SharedBaked> {
  if (auto const found{ _slots.find(key) }; found != _slots.end()) {
    ++_statistics.hits;
    _recent.splice(_recent.begin(), _recent, found->second);
    return found->second->baked;
  }
  ++_statistics.misses;
  auto baked{ Bake(key) };
  if (!baked) return std::unexpected{ baked.error() };
  Remember(key, std::make_shared<Baked const>(*std::move(baked)));
  return _recent.front().baked;
}

auto TextureCache::Bake(TextureKey const& key) const -> Rendered<Baked> {
  auto const asset{ _pack.get().Load(key.asset) };
  if (!asset) return PackFailed(asset.error());
  auto const pixels{ Pixels(*asset, key.palette) };
  if (!pixels) return std::unexpected{ pixels.error() };
  auto texture{ Texture::Upload(_renderer.get(), *pixels) };
  if (!texture) return std::unexpected{ texture.error() };
  return Baked{ .texture = *std::move(texture), .size = GameSize(asset->entry) };
}

auto TextureCache::Pixels(assets::LoadedAsset const& asset, std::string_view const palette) const -> Rendered<assets::Texture> {
  if (palette.empty()) return asset.texture;
  auto const colours{ _pack.get().Palette(palette) };
  if (!colours) return Failed(std::format("no palette {}", palette));
  return render::Bake(asset, *colours);
}

auto TextureCache::Remember(TextureKey const& key, SharedBaked baked) -> void {
  _recent.push_front(Slot{ .key = key, .baked = std::move(baked) });
  _slots.insert_or_assign(key, _recent.begin());
  if (_recent.size() <= _capacity) return;
  _slots.erase(_recent.back().key);
  _recent.pop_back();
  ++_statistics.evictions;
}

}
