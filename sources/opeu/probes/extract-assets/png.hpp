#pragma once
// Writing an asset as PNG: the picture in its palette (transparent where its mask says so) and its index map.

#include "opeu/assets/image.hpp"
#include "opeu/assets/palette.hpp"

#include <expected>
#include <filesystem>
#include <string>

namespace opeu::probes::extract_assets::detail::png {

// RGBA, alpha 0 where the image's mask keeps the background.
[[nodiscard]] auto WriteColour(std::filesystem::path const& path, assets::Image const& image,
                               assets::NamedPalette const& palette) -> std::expected<void, std::string>;

// One grey byte per pixel: the palette index, 255 where the mask keeps the background.
[[nodiscard]] auto WriteIndices(std::filesystem::path const& path, assets::Image const& image)
    -> std::expected<void, std::string>;

}

namespace opeu::probes::extract_assets {
using detail::png::WriteColour;
using detail::png::WriteIndices;
}
