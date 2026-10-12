#include "png.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <ranges>
#include <vector>

// stb_image_write is a single header whose implementation is compiled where this macro is set; static keeps its
// symbols out of the way of the platform layer's copy.
#define STB_IMAGE_WRITE_STATIC
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace opeu::probes::extract_assets::detail::png {

namespace {

auto Write(std::filesystem::path const& path, assets::Image const& image, int const channels,
           std::vector<std::uint8_t> const& bytes) -> std::expected<void, std::string> {
  std::filesystem::create_directories(path.parent_path());
  auto const width{ static_cast<int>(image.Width()) };
  if (stbi_write_png(path.string().c_str(), width, static_cast<int>(image.Height()), channels, bytes.data(),
                     width * channels)
      == 0) {
    return std::unexpected{ std::format("extract-assets: cannot write {}", path.string()) };
  }
  return {};
}

// Each pixel with whether it is drawn, row-major.
auto PixelsAndMask(assets::Image const& image) {
  return std::views::zip(image.Pixels(), image.HasMask()
                                             ? std::vector<std::uint8_t>(image.Opaque().begin(), image.Opaque().end())
                                             : std::vector<std::uint8_t>(image.Pixels().size(), 1));
}

}

auto WriteColour(std::filesystem::path const& path, assets::Image const& image, assets::NamedPalette const& palette)
    -> std::expected<void, std::string> {
  auto const colours{ assets::Colours(palette.levels) };
  auto const rgba{ PixelsAndMask(image) | std::views::transform([&colours](auto const pixel) {
                     auto const [index, drawn]{ pixel };
                     auto const colour{ colours[index & 0x0F] };
                     return std::array<std::uint8_t, 4>{ colour.r, colour.g, colour.b,
                                                         static_cast<std::uint8_t>(drawn != 0 ? 255 : 0) };
                   })
                   | std::views::join | std::ranges::to<std::vector>() };
  return Write(path, image, 4, rgba);
}

auto WriteIndices(std::filesystem::path const& path, assets::Image const& image) -> std::expected<void, std::string> {
  auto const grey{ PixelsAndMask(image) | std::views::transform([](auto const pixel) {
                     auto const [index, drawn]{ pixel };
                     return drawn != 0 ? index : std::uint8_t{ 255 };
                   })
                   | std::ranges::to<std::vector>() };
  return Write(path, image, 1, grey);
}

}
