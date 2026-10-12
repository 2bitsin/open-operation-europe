#include "opeu/assets/texture.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::texture {

Texture::Texture(std::size_t const width, std::size_t const height, std::vector<std::byte> rgba)
    : _width{ width }, _height{ height }, _rgba{ std::move(rgba) } {
  Expects(_rgba.size() == width * height * 4, "a texture has four bytes per pixel");
}

auto ToTexture(Image const& image, std::array<Colour, 16> const& colours) -> Texture {
  auto const pixels{ Indices(image.Pixels().size()) | std::views::transform([&](std::size_t const n) {
                       auto const colour{ colours.at(image.Pixels()[n] & 0x0F) };
                       auto const drawn{ image.Opaque().empty() || image.Opaque()[n] != 0 };
                       return std::array{ std::byte{ colour.r }, std::byte{ colour.g }, std::byte{ colour.b },
                                          drawn ? std::byte{ 0xFF } : std::byte{ 0 } };
                     })
                     | std::views::join | std::ranges::to<std::vector>() };
  return Texture{ image.Width(), image.Height(), pixels };
}

auto ToTexture(Image const& image, NamedPalette const& palette) -> Texture {
  return ToTexture(image, Colours(palette.levels));
}

}
