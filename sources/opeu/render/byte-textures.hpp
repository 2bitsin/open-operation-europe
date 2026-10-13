#pragma once
// Textures of the images the game hands over as bytes in its memory. Bytes that are a shipped asset unchanged (the
// content index recognises them) are drawn from the pack, by id, so a remaster pack replaces them; any other bytes are
// decoded once with the assets module's decoders and kept by their fingerprint. Either way a texture is baked in the
// colours the screen shows, keyed or not, and kept until it is the least recently used past the capacity.

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/render/baking.hpp"
#include "opeu/render/recent-store.hpp"
#include "opeu/render/render-error.hpp"
#include "opeu/render/texture-cache.hpp"

#include <SDL3/SDL.h>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

namespace opeu::render::detail::byte_textures {

// An image as the game's bytes: how its planes lie, the bytes (at least the layout's), and the catalogue layout of
// the shipped assets those bytes may be (none for a layout no asset is stored in, a battle cell buffer say).
struct GameImage {
  assets::PlaneLayout           layout;
  assets::Bytes                 bytes;
  std::optional<assets::Layout> shipped;
};

// How a texture is painted: the colour each number shows, and whether colour 0 lets the screen show.
struct Paint {
  assets::PaletteColours colours{};
  Keying                 keying{ Keying::OPAQUE };

  auto operator<=>(Paint const&) const = default;
};

struct ByteStatistics {
  std::size_t     recognised{}; // drawn from the pack by the id the bytes are
  std::size_t     unpacked{};   // recognised, but the pack holds no such id: decoded instead
  CacheStatistics decoded{};    // the decoded images' store

  auto operator==(ByteStatistics const&) const -> bool = default;
};

class ByteTextures {
public:
  // The pack's textures come from `pack_textures`; `index` says which bytes are which asset. Both outlive this.
  ByteTextures(SDL_Renderer& renderer, TextureCache& pack_textures, assets::ContentIndex const& index, std::size_t capacity);

  // The texture of `image` painted with `paint`, drawn at the game's size its Baked says.
  [[nodiscard]] auto Fetch(GameImage const& image, Paint const& paint) -> Rendered<SharedBaked>;
  // The texture of a picture as its stream drew it, at its drawn size (its rows' overrun included).
  [[nodiscard]] auto Fetch(assets::DrawnPicture const& picture, Paint const& paint) const -> Rendered<SharedBaked>;
  // The pack's texture of the shipped picture the stream is, drawn at the picture's own size; none for another stream.
  [[nodiscard]] auto FetchShipped(assets::DrawnPicture const& picture, assets::Layout stream, Paint const& paint) -> std::optional<SharedBaked>;

  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics;

private:
  struct DecodedKey {
    assets::PlaneLayout layout;
    std::uint64_t       fingerprint{};
    Paint               paint;

    auto operator<=>(DecodedKey const&) const = default;
  };

  // The pack's texture of the asset `key` is, when the index knows it and the pack holds it.
  [[nodiscard]] auto FromPack(assets::ContentKey const& key, Paint const& paint) -> std::optional<SharedBaked>;
  [[nodiscard]] auto Decoded(GameImage const& image, std::uint64_t fingerprint, Paint const& paint) -> Rendered<SharedBaked>;
  [[nodiscard]] auto Uploaded(assets::Image const& image, Paint const& paint) const -> Rendered<SharedBaked>;

  std::reference_wrapper<SDL_Renderer>               _renderer;
  std::reference_wrapper<TextureCache>               _pack_textures;
  std::reference_wrapper<assets::ContentIndex const> _index;
  RecentStore<DecodedKey, SharedBaked>               _decoded;
  std::size_t                                        _recognised{};
  std::size_t                                        _unpacked{};
};

}

namespace opeu::render {
using detail::byte_textures::ByteStatistics;
using detail::byte_textures::ByteTextures;
using detail::byte_textures::GameImage;
using detail::byte_textures::Paint;
}
