#pragma once
// Writing an asset as PNG: its texture (RGBA) and its index map.

#include "opeu/assets/image.hpp"
#include "opeu/assets/texture.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace opeu::probes::extract_assets::detail::png {

[[nodiscard]] auto WriteTexture(std::filesystem::path const& path, assets::Texture const& texture) -> std::expected<void, std::string>;

// One grey byte per pixel: the palette index, 255 where the mask lets the background show.
[[nodiscard]] auto WriteIndices(std::filesystem::path const& path, assets::Image const& image) -> std::expected<void, std::string>;

}

namespace opeu::probes::extract_assets {
using detail::png::WriteIndices;
using detail::png::WriteTexture;
}
