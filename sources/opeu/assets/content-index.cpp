#include "opeu/assets/content-index.hpp"

#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/picture-stream.hpp"

#include <algorithm>

namespace opeu::assets::detail::content_index {

namespace {

[[nodiscard]] constexpr auto PlanesOf(Layout const layout) noexcept -> std::size_t {
  switch (layout) {
  case Layout::PLANES_4: return 4;
  case Layout::PLANES_3: return 3;
  case Layout::BITMAP:   return 1;
  default:               return 0;
  }
}

// The asset's bytes as the game draws them, when it draws them unchanged: a raw image's record, a stream up to its end.
[[nodiscard]] auto KeyOf(Bytes const file, Asset const& asset) -> std::optional<ContentKey> {
  if (asset.layout == Layout::STREAM_8 || asset.layout == Layout::STREAM_16) {
    auto const colours{ asset.layout == Layout::STREAM_8 ? Colours::EIGHT : Colours::SIXTEEN };
    auto const length{ StreamLength(file.subspan(std::min<std::size_t>(asset.at, file.size())), colours) };
    if (!length) return std::nullopt;
    return StreamKey(asset.layout, FingerprintOf(file.subspan(asset.at, *length)));
  }
  // A one-bit shape inked where its bits are clear, or narrower than its bytes, is not what the set bits draw.
  if (asset.layout == Layout::BITMAP && (asset.ink_where_clear || (asset.width != 0 && asset.width != asset.width_bytes * 8U))) return std::nullopt;
  auto const planes{ PlanesOf(asset.layout) };
  if (planes == 0) return std::nullopt;
  auto const record{ Slice(file, asset.at, std::size_t{ asset.width_bytes } * asset.height * planes) };
  if (!record) return std::nullopt;
  return RawKey(RawShape{ .layout = asset.layout, .width_bytes = asset.width_bytes, .height = asset.height }, *record);
}

}

auto RawKey(RawShape const& shape, Bytes const bytes) -> ContentKey {
  return ContentKey{ .layout = shape.layout, .width_bytes = shape.width_bytes, .height = shape.height, .fingerprint = FingerprintOf(bytes) };
}

auto StreamKey(Layout const layout, std::uint64_t const fingerprint) -> ContentKey {
  return ContentKey{ .layout = layout, .width_bytes = 0, .height = 0, .fingerprint = fingerprint };
}

ContentIndex::ContentIndex(Catalogue const& catalogue) {
  std::ranges::for_each(catalogue.Assets(), [&](Asset const& asset) { Add(catalogue, asset); });
}

auto ContentIndex::Recognise(ContentKey const& key) const -> std::optional<std::string_view> {
  if (auto const found{ _ids.find(key) }; found != _ids.end()) return found->second;
  return std::nullopt;
}

auto ContentIndex::Add(Catalogue const& catalogue, Asset const& asset) -> void {
  auto const file{ catalogue.File(asset.file) };
  if (!file) return;
  if (auto const key{ KeyOf(*file, asset) }) _ids.try_emplace(*key, asset.id);
}

}
