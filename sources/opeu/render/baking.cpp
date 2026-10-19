#include "opeu/render/baking.hpp"

#include "opeu/render/rows.hpp"

#include <array>
#include <ranges>
#include <vector>

namespace opeu::render::detail::baking {

auto Bake(assets::LoadedAsset const& asset, assets::PaletteColours const& colours) -> assets::Texture {
  return Bake(asset, Painting{ .colours = colours, .keying = Keying::OPAQUE, .background = std::nullopt });
}

auto Bake(assets::LoadedAsset const& asset, Painting const& painting) -> assets::Texture {
  if (asset.indexed) return assets::ToTexture(KeyedImage(*asset.indexed, painting.keying), painting.colours);
  return KeyedImage(asset.texture, painting.keying, painting.background);
}

auto KeyedImage(assets::Image const& image, Keying const keying) -> assets::Image {
  if (keying == Keying::OPAQUE) return image;
  auto opaque{ assets::Indices(image.Pixels().size())
               | std::views::transform([&image](std::size_t const n) { return static_cast<std::uint8_t>(image.Pixels()[n] != 0 && (!image.HasMask() || image.Opaque()[n] != 0)); })
               | std::ranges::to<std::vector>() };
  return assets::Image{ image.Shape(), image.Pixels() | std::ranges::to<std::vector>(), std::move(opaque) };
}

auto KeyedImage(assets::Texture const& image, Keying const keying, std::optional<assets::Colour> const background) -> assets::Texture {
  if (keying == Keying::OPAQUE || !background) return image;
  auto const keyed{ [colour = *background](auto const pixel) {
    auto const shows{
      std::array{ pixel[0],              pixel[1],              pixel[2]              }
      == std::array{ std::byte{ colour.r }, std::byte{ colour.g }, std::byte{ colour.b } }
    };
    return std::array{ pixel[0], pixel[1], pixel[2], shows ? std::byte{ 0 } : pixel[3] };
  } };
  auto const pixels{ Rows(image.Pixels(), RowLayout{ .pitch = 4, .width = 4, .count = image.Width() * image.Height() }) };
  return assets::Texture{ image.Width(), image.Height(), pixels | std::views::transform(keyed) | std::views::join | std::ranges::to<std::vector>() };
}

auto Pen(assets::PaletteColours const& colours, std::size_t const index) -> Rgba {
  auto const colour{ colours.at(index) };
  return Rgba{ .r = colour.r, .g = colour.g, .b = colour.b };
}

}
