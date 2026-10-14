#include "opeu/render/byte-textures.hpp"

#include "opeu/assets/fingerprint.hpp"

#include <memory>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

namespace opeu::render::detail::byte_textures {
namespace {

// Pictures are large and few on a screen: a handful of colourings each.
constexpr auto PICTURES{ 64uz };

// A one-plane image is a shape: its set bits drawn, its clear bits letting the screen show.
[[nodiscard]] auto AsShape(assets::Image const& bits) -> assets::Image {
  return assets::Image{ bits.Width(), bits.Height(), bits.Pixels() | std::ranges::to<std::vector>(), bits.Pixels() | std::ranges::to<std::vector>() };
}

// The numbers of `image` with every row on `times` lines.
[[nodiscard]] auto RowsRepeated(assets::Image const& image, std::size_t const times) -> assets::Image {
  if (times == 1) return image;
  auto const rows{ assets::Indices(image.Height() * times) | std::views::transform([&](std::size_t const row) { return image.Pixels().subspan(row / times * image.Width(), image.Width()); }) };
  return assets::Image{ image.Width(), image.Height() * times, rows | std::views::join | std::ranges::to<std::vector>() };
}

[[nodiscard]] auto SizeOf(assets::Image const& image) -> Size {
  return Size{ .width = static_cast<int>(image.Width()), .height = static_cast<int>(image.Height()) };
}

}

ByteTextures::ByteTextures(SDL_Renderer& renderer, TextureCache& pack_textures, assets::ContentIndex const& index, std::size_t const capacity)
    : _renderer{ renderer }, _pack_textures{ pack_textures }, _index{ index }, _decoded{ capacity }, _pictures{ PICTURES } { }

auto ByteTextures::Fetch(GameImage const& image, Paint const& paint) -> Rendered<SharedBaked> {
  auto const bytes{ assets::Need(image.bytes, 0, image.layout.Bytes()) };
  if (!bytes) return Failed("the image's bytes end before the image does");
  auto const fingerprint{ assets::FingerprintOf(*bytes) };
  if (image.shipped && image.repeat == 1) {
    auto const key{
      assets::ContentKey{ .layout = *image.shipped, .width_bytes = image.layout.width_bytes, .height = image.layout.height, .fingerprint = fingerprint }
    };
    if (auto packed{ FromPack(key, paint) }) return *std::move(packed);
  }
  return Decoded(GameImage{ .layout = image.layout, .bytes = *bytes, .shipped = image.shipped, .repeat = image.repeat }, fingerprint, paint);
}

auto ByteTextures::Fetch(assets::DrawnPicture const& picture, Paint const& paint) -> Rendered<SharedBaked> {
  auto const key{
    DecodedKey{ .layout = assets::InterleavedLayout(picture.image.Width(), picture.image.Height(), 0), .fingerprint = picture.fingerprint, .repeat = 1, .paint = paint }
  };
  if (auto found{ _pictures.Find(key) }) return *std::move(found);
  auto baked{ Uploaded(picture.image, paint) };
  if (baked) _pictures.Remember(key, *baked);
  return baked;
}

auto ByteTextures::FetchShipped(assets::DrawnPicture const& picture, assets::Layout const stream, Paint const& paint) -> std::optional<SharedBaked> {
  return FromPack(assets::StreamKey(stream, picture.fingerprint), paint);
}

auto ByteTextures::Statistics() const noexcept -> ByteStatistics {
  return ByteStatistics{ .recognised = _recognised, .unpacked = _unpacked, .decoded = _decoded.Statistics() };
}

auto ByteTextures::FromPack(assets::ContentKey const& key, Paint const& paint) -> std::optional<SharedBaked> {
  auto const id{ _index.get().Recognise(key) };
  if (!id) return std::nullopt;
  auto baked{ _pack_textures.get().Fetch(TextureKey{ .asset = std::string{ *id }, .palette = {}, .colours = paint.colours, .keying = paint.keying }) };
  if (!baked) {
    ++_unpacked;
    return std::nullopt;
  }
  ++_recognised;
  return *std::move(baked);
}

auto ByteTextures::Decoded(GameImage const& image, std::uint64_t const fingerprint, Paint const& paint) -> Rendered<SharedBaked> {
  auto const key{
    DecodedKey{ .layout = image.layout, .fingerprint = fingerprint, .repeat = image.repeat, .paint = paint }
  };
  if (auto found{ _decoded.Find(key) }) return *std::move(found);
  auto const decoded{ assets::DecodePlanes(image.bytes, image.layout) };
  if (!decoded) return Failed(std::string{ assets::Describe(decoded.error()) });
  auto const repeated{ RowsRepeated(*decoded, image.repeat) };
  auto       baked{ Uploaded(image.layout.planes == 1 ? AsShape(repeated) : repeated, paint) };
  if (baked) _decoded.Remember(key, *baked);
  return baked;
}

auto ByteTextures::Uploaded(assets::Image const& image, Paint const& paint) const -> Rendered<SharedBaked> {
  auto texture{ Texture::Upload(_renderer.get(), assets::ToTexture(KeyedImage(image, paint.keying), paint.colours)) };
  if (!texture) return std::unexpected{ texture.error() };
  return std::make_shared<Baked const>(Baked{ .texture = *std::move(texture), .size = SizeOf(image) });
}

}
