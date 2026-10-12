#pragma once
// An image ready for the screen: 32-bit pixels in memory order R, G, B, A (SDL_PIXELFORMAT_RGBA32, which is
// SDL_PIXELFORMAT_ABGR8888 on little-endian machines), alpha 0 where the image's mask lets the background show.
// SDL_UpdateTexture takes Pixels().data() and Pitch() as they are.

#include "opeu/assets/image.hpp"
#include "opeu/assets/palette.hpp"

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace opeu::assets::detail::texture {

class Texture {
public:
  Texture() = default;
  Texture(std::size_t width, std::size_t height, std::vector<std::byte> rgba);

  [[nodiscard]] auto Width() const noexcept -> std::size_t {
    return _width;
  }
  [[nodiscard]] auto Height() const noexcept -> std::size_t {
    return _height;
  }
  [[nodiscard]] auto Pitch() const noexcept -> std::size_t {
    return _width * 4;
  } // bytes per row
  [[nodiscard]] auto Pixels() const noexcept -> std::span<std::byte const> {
    return _rgba;
  }

  auto operator==(Texture const&) const -> bool = default;

private:
  std::size_t            _width{};
  std::size_t            _height{};
  std::vector<std::byte> _rgba;
};

// The image in `colours` (an index past 15 uses its low four bits).
[[nodiscard]] auto ToTexture(Image const& image, std::array<Colour, 16> const& colours) -> Texture;
[[nodiscard]] auto ToTexture(Image const& image, NamedPalette const& palette) -> Texture;

}

namespace opeu::assets {
using detail::texture::Texture;
using detail::texture::ToTexture;
}
