#include "opeu/assets/image.hpp"

#include "opeu/contract.hpp"

#include <utility>

namespace opeu::assets::detail::image {

Image::Image(std::size_t const width, std::size_t const height, std::vector<std::uint8_t> pixels, std::vector<std::uint8_t> opaque)
    : _width{ width }, _height{ height }, _pixels{ std::move(pixels) }, _opaque{ std::move(opaque) } {
  Expects(_pixels.size() == width * height, "an image has one index per pixel");
  Expects(_opaque.empty() || _opaque.size() == _pixels.size(), "a mask has one entry per pixel");
}

}
