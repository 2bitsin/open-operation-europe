#pragma once
// Textures baked from the pack, one per (asset id, palette), kept in a bounded least-recently-used cache. A palette
// change (an hour, a scene of the opening) is a different key: the textures of the previous palette age out.

#include "opeu/render/geometry.hpp"
#include "opeu/render/pack-source.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/texture.hpp"

#include <SDL3/SDL.h>

#include <compare>
#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>

namespace opeu::render::detail::texture_cache {

struct TextureKey {
  std::string asset{};
  std::string palette{}; // empty: the asset's own image, as the pack holds it

  auto operator<=>(TextureKey const&) const = default;
};

// A texture and the size the game draws it at (its image is the pack's scale times that).
struct Baked {
  Texture texture;
  Size    size;
};

using SharedBaked = std::shared_ptr<Baked const>;

struct CacheStatistics {
  std::size_t hits{};
  std::size_t misses{};
  std::size_t evictions{};

  auto operator==(CacheStatistics const&) const -> bool = default;
};

class TextureCache {
public:
  TextureCache(SDL_Renderer& renderer, PackSource const& source, std::size_t capacity);

  // The texture for `key`, baked on a miss. It stays valid while held, even after the cache lets it go.
  [[nodiscard]] auto Fetch(TextureKey const& key) -> Rendered<SharedBaked>;
  [[nodiscard]] auto Statistics() const noexcept -> CacheStatistics {
    return _statistics;
  }
  [[nodiscard]] auto Holds(TextureKey const& key) const -> bool {
    return _slots.contains(key);
  }
  [[nodiscard]] auto Source() const noexcept -> PackSource const& {
    return _source.get();
  }

private:
  struct Slot {
    TextureKey  key;
    SharedBaked baked;
  };
  using Recent = std::list<Slot>;

  [[nodiscard]] auto Bake(TextureKey const& key) const -> Rendered<Baked>;
  auto               Remember(TextureKey const& key, SharedBaked baked) -> void;

  std::reference_wrapper<SDL_Renderer>                _renderer;
  std::reference_wrapper<PackSource const>            _source;
  std::size_t                                         _capacity;
  Recent                                              _recent; // most recently used first
  std::map<TextureKey, Recent::iterator, std::less<>> _slots;
  CacheStatistics                                     _statistics;
};

}

namespace opeu::render {
using detail::texture_cache::Baked;
using detail::texture_cache::CacheStatistics;
using detail::texture_cache::SharedBaked;
using detail::texture_cache::TextureCache;
using detail::texture_cache::TextureKey;
}
