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

// Pixels to write: rows of `width` pixels of `channels` bytes each.
struct Raster {
  std::size_t                width;
  std::size_t                height;
  int                        channels;
  std::span<std::byte const> bytes;
};

auto Write(std::filesystem::path const& path, Raster const& raster) -> std::expected<void, std::string> {
  std::filesystem::create_directories(path.parent_path());
  auto const width{ static_cast<int>(raster.width) };
  if (stbi_write_png(path.string().c_str(), width, static_cast<int>(raster.height), raster.channels, raster.bytes.data(), width * raster.channels) == 0) {
    return std::unexpected{ std::format("extract-assets: cannot write {}", path.string()) };
  }
  return {};
}

}

auto WriteTexture(std::filesystem::path const& path, assets::Texture const& texture) -> std::expected<void, std::string> {
  return Write(path, { .width = texture.Width(), .height = texture.Height(), .channels = 4, .bytes = texture.Pixels() });
}

auto WriteIndices(std::filesystem::path const& path, assets::Image const& image) -> std::expected<void, std::string> {
  auto const grey{ assets::Indices(image.Pixels().size()) | std::views::transform([&image](std::size_t const n) {
                     auto const drawn{ image.Opaque().empty() || image.Opaque()[n] != 0 };
                     return drawn ? std::byte{ image.Pixels()[n] } : std::byte{ 0xFF };
                   })
                   | std::ranges::to<std::vector>() };
  return Write(path, { .width = image.Width(), .height = image.Height(), .channels = 1, .bytes = grey });
}

}
