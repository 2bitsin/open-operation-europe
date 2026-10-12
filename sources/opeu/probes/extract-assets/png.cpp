// census: raii — stb_image_write takes the pixels as a const void* and the path as a C string; both come from the
// owning vector and path of the call.
#include "png.hpp"

#include "opeu/assets/bytes.hpp"

#include <format>
#include <ranges>
#include <vector>

// configure.py compiles stb_image_write's implementation into this file, static.
#include <stb_image_write.h>

namespace opeu::probes::extract_assets::detail::png {

namespace {

auto Write(std::filesystem::path const& path, std::size_t const width, std::size_t const height, int const channels,
           std::span<std::byte const> const bytes) -> std::expected<void, std::string> {
  std::filesystem::create_directories(path.parent_path());
  if (stbi_write_png(path.string().c_str(), static_cast<int>(width), static_cast<int>(height), channels, bytes.data(),
                     static_cast<int>(width) * channels)
      == 0) {
    return std::unexpected{ std::format("extract-assets: cannot write {}", path.string()) };
  }
  return {};
}

}

auto WriteTexture(std::filesystem::path const& path, assets::Texture const& texture)
    -> std::expected<void, std::string> {
  return Write(path, texture.Width(), texture.Height(), 4, texture.Pixels());
}

auto WriteIndices(std::filesystem::path const& path, assets::Image const& image) -> std::expected<void, std::string> {
  auto const grey{ assets::Indices(image.Pixels().size()) | std::views::transform([&image](std::size_t const n) {
                     auto const drawn{ image.Opaque().empty() || image.Opaque()[n] != 0 };
                     return drawn ? std::byte{ image.Pixels()[n] } : std::byte{ 0xFF };
                   })
                   | std::ranges::to<std::vector>() };
  return Write(path, image.Width(), image.Height(), 1, grey);
}

}
