#pragma once
// Textures baked from an asset pack (a built pack, a remaster over it, or the decoders' CataloguePack: the same
// interface), one per (asset id, palette or colours, keying), kept in a bounded least-recently-used cache. A palette
// change (an hour, a scene of the opening, colours the game sets entry by entry) is a different key: the textures of
// the previous palette age out.

#include "opeu/assets/pack.hpp"
#include "opeu/render/baking.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/recent-store.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/texture.hpp"

#include <SDL3/SDL.h>

#include <compare>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace opeu::render::detail::texture_cache {

struct TextureKey {
  std::string                           asset;
  std::string                           palette;                  // empty: the asset's own image, as the pack holds it
  std::optional<assets::PaletteColours> colours;                  // colours rather than a named palette: they win
  Keying                                keying{ Keying::OPAQUE }; // whether colour 0 lets the screen show

  auto operator<=>(TextureKey const&) const = default;
};

// A texture and the size the game draws it at (its image is the pack's scale times that).
struct Baked {
  Texture texture;
  Size    size;
};

using SharedBaked = std::shared_ptr<Baked const>;

class TextureCache {
public:
  TextureCache(SDL_Renderer& renderer, assets::AssetPack const& pack, std::size_t capacity);

  // The texture for `key`, baked on a miss. It stays valid while held, even after the cache lets it go.
  [[nodiscard]] auto Fetch(TextureKey const& key) -> Rendered<SharedBaked>;
  [[nodiscard]] auto Statistics() const noexcept -> CacheStatistics {
    return _baked.Statistics();
  }
  [[nodiscard]] auto Holds(TextureKey const& key) const -> bool {
    return _baked.Holds(key);
  }
  [[nodiscard]] auto Pack() const noexcept -> assets::AssetPack const& {
    return _pack.get();
  }

private:
  [[nodiscard]] auto Bake(TextureKey const& key) const -> Rendered<Baked>;
  // The asset's image as the pack holds it (no palette or colours), or baked in the colours; keyed as asked.
  [[nodiscard]] auto Pixels(assets::LoadedAsset const& asset, TextureKey const& key) const -> Rendered<assets::Texture>;
  [[nodiscard]] auto ColoursOf(TextureKey const& key) const -> Rendered<std::optional<assets::PaletteColours>>;

  std::reference_wrapper<SDL_Renderer>            _renderer;
  std::reference_wrapper<assets::AssetPack const> _pack;
  RecentStore<TextureKey, SharedBaked>            _baked;
};

}

namespace opeu::render {
using detail::texture_cache::Baked;
using detail::texture_cache::SharedBaked;
using detail::texture_cache::TextureCache;
using detail::texture_cache::TextureKey;
}
