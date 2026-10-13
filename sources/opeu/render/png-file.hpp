#pragma once
// An RGBA image written as a PNG file (SDL's own encoder): the headless frame captures and the test goldens.

#include "opeu/assets/texture.hpp"
#include "opeu/render/render-error.hpp"

#include <filesystem>

namespace opeu::render::detail::png_file {

[[nodiscard]] auto WritePng(assets::Texture const& image, std::filesystem::path const& path) -> Rendered<void>;

}

namespace opeu::render {
using detail::png_file::WritePng;
}
