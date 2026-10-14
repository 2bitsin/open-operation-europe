#pragma once
// Which shipped asset a run of bytes is. The game hands the renderer images as bytes in its memory (a tile, a
// portrait, a glyph, a picture stream); when those bytes are exactly an asset of the catalogue, the renderer draws the
// pack's image of that id instead of decoding them, so a remaster pack replaces it. Bytes the game composes itself
// (battle cells, saved screen rectangles) match nothing and are decoded.

#include "opeu/assets/catalogue.hpp"

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace opeu::assets::detail::content_index {

// Bytes as the game draws them: the layout, the size a raw layout is drawn at (0 for a stream, which states its own),
// and the bytes' fingerprint.
struct ContentKey {
  Layout        layout{};
  std::size_t   width_bytes{};
  std::size_t   height{};
  std::uint64_t fingerprint{};

  auto operator<=>(ContentKey const&) const = default;
};

// The shape of a raw image (PLANES_4, PLANES_3, BITMAP): its layout, `width_bytes` byte columns by `height` rows.
struct RawShape {
  Layout      layout{};
  std::size_t width_bytes{};
  std::size_t height{};
};

// The key of a raw image of `shape` drawn from `bytes`.
[[nodiscard]] auto RawKey(RawShape const& shape, Bytes bytes) -> ContentKey;
// The key of a picture stream (STREAM_8, STREAM_16) whose bytes fingerprint as `fingerprint`.
[[nodiscard]] auto StreamKey(Layout layout, std::uint64_t fingerprint) -> ContentKey;

class ContentIndex {
public:
  // Recognises nothing: every image is decoded from its bytes.
  ContentIndex() = default;
  // Every catalogue asset the game draws from its own bytes unchanged: column images, full-width one-bit shapes drawn
  // with their set bits, picture streams. Identical bytes answer the first such asset.
  explicit ContentIndex(Catalogue const& catalogue);

  [[nodiscard]] auto Recognise(ContentKey const& key) const -> std::optional<std::string_view>;
  [[nodiscard]] auto Size() const noexcept -> std::size_t {
    return _ids.size();
  }

private:
  auto Add(Catalogue const& catalogue, Asset const& asset) -> void;

  std::map<ContentKey, std::string, std::less<>> _ids;
};

}

namespace opeu::assets {
using detail::content_index::ContentIndex;
using detail::content_index::ContentKey;
using detail::content_index::RawKey;
using detail::content_index::RawShape;
using detail::content_index::StreamKey;
}
