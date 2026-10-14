#pragma once
// An indexed picture as every decoder returns it, and why a decode can fail.

#include "opeu/assets/bytes.hpp"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::image {

// An indexed picture: palette indices row-major, and optionally which pixels are drawn (a sprite's mask). The
// sizes always agree; an image without a mask draws every pixel.
class Image {
public:
  Image() = default;
  Image(std::size_t width, std::size_t height, std::vector<std::uint8_t> pixels, std::vector<std::uint8_t> opaque = {});

  [[nodiscard]] auto Width() const noexcept -> std::size_t {
    return _width;
  }
  [[nodiscard]] auto Height() const noexcept -> std::size_t {
    return _height;
  }
  [[nodiscard]] auto At(std::size_t const x, std::size_t const y) const -> std::uint8_t {
    return _pixels[(y * _width) + x];
  }
  [[nodiscard]] auto Drawn(std::size_t const x, std::size_t const y) const -> bool {
    return _opaque.empty() || _opaque[(y * _width) + x] != 0;
  }
  [[nodiscard]] auto HasMask() const noexcept -> bool {
    return !_opaque.empty();
  }
  [[nodiscard]] auto Pixels() const noexcept -> std::span<std::uint8_t const> {
    return _pixels;
  }
  // Empty without a mask, else one entry per pixel: 1 drawn, 0 the background shows.
  [[nodiscard]] auto Opaque() const noexcept -> std::span<std::uint8_t const> {
    return _opaque;
  }

  // The same picture with every index passed through `map` (a recolouring); the mask is kept.
  template <class _Map>
  [[nodiscard]] auto Recoloured(_Map map) const -> Image {
    auto copy{ *this };
    for (auto& index : copy._pixels) index = static_cast<std::uint8_t>(map(index));
    return copy;
  }

  auto operator==(Image const&) const -> bool = default;

private:
  std::size_t               _width{};
  std::size_t               _height{};
  std::vector<std::uint8_t> _pixels;
  std::vector<std::uint8_t> _opaque;
};

enum class DecodeError : std::uint8_t {
  TRUNCATED,    // the bytes end before the record does
  BAD_HEADER,   // a picture stream's size is outside the 640x400 screen
  BAD_SIZE,     // a file whose size is not the shipped format's
  OUT_OF_RANGE, // an index or offset past the end of the file
  MISSING_FILE, // the source has no file of that name
  BAD_LAYOUT,   // a music bank whose table, instruments, drum kits and tracks do not tile it end to end
};

template <class _Value>
using Decoded = std::expected<_Value, DecodeError>;

[[nodiscard]] constexpr auto Describe(DecodeError const error) noexcept -> std::string_view {
  switch (error) {
  case DecodeError::TRUNCATED:    return "truncated";
  case DecodeError::BAD_HEADER:   return "bad picture header";
  case DecodeError::BAD_SIZE:     return "unexpected file size";
  case DecodeError::OUT_OF_RANGE: return "out of range";
  case DecodeError::MISSING_FILE: return "missing file";
  case DecodeError::BAD_LAYOUT:   return "bad music bank layout";
  }
  return "unknown";
}

// `count` bytes from `at`, or TRUNCATED.
[[nodiscard]] constexpr auto Need(Bytes const bytes, std::size_t const at, std::size_t const count) -> Decoded<Bytes> {
  if (auto const slice{ Slice(bytes, at, count) }) return *slice;
  return std::unexpected{ DecodeError::TRUNCATED };
}

}

namespace opeu::assets {
using detail::image::Decoded;
using detail::image::DecodeError;
using detail::image::Describe;
using detail::image::Image;
using detail::image::Need;
}
