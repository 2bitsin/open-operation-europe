// census: raii — stb_image hands out its pixels as a malloc'd unsigned char*, owned here by a unique_ptr whose
// deleter is stbi_image_free, and takes the PNG as a pointer and length from a copy in its own byte type.
#include "opeu/assets/png-reader.hpp"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <ranges>
#include <span>
#include <vector>

// configure.py compiles stb_image's implementation (PNG only) into this file, static.
#include <stb_image.h>

namespace opeu::assets::detail::png_reader {

namespace {

struct StbFree {
  auto operator()(stbi_uc* const pixels) const noexcept -> void {
    stbi_image_free(pixels);
  }
};

struct Decoded8 {
  std::size_t               width{};
  std::size_t               height{};
  std::vector<std::uint8_t> samples; // `channels` per pixel
};

auto Decode(Bytes const png, int const channels) -> Packed<Decoded8> {
  auto       width{ 0 };
  auto       height{ 0 };
  auto       found{ 0 };
  auto const encoded{ png | std::views::transform([](std::byte const b) { return std::to_integer<stbi_uc>(b); }) | std::ranges::to<std::vector>() };
  auto const pixels{ std::unique_ptr<stbi_uc, StbFree>{ stbi_load_from_memory(encoded.data(), static_cast<int>(encoded.size()), &width, &height, &found, channels) } };
  if (!pixels) return Fail(PackFault::BAD_IMAGE, stbi_failure_reason());
  auto const count{ static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * static_cast<std::size_t>(channels) };
  auto const samples{
    std::span{ pixels.get(), count }
  };
  return Decoded8{
    .width = static_cast<std::size_t>(width), .height = static_cast<std::size_t>(height), .samples = { samples.begin(), samples.end() }
  };
}

}

auto ReadTexture(Bytes const png) -> Packed<Texture> {
  return Decode(png, 4).transform([](Decoded8 const& rgba) {
    return Texture{ rgba.width, rgba.height, rgba.samples | std::views::transform([](std::uint8_t const s) { return std::byte{ s }; }) | std::ranges::to<std::vector>() };
  });
}

auto ReadIndexed(Bytes const png) -> Packed<Image> {
  return Decode(png, 1).transform([](Decoded8 const& grey) {
    auto const drawn{ [](std::uint8_t const index) { return index != TRANSPARENT_INDEX; } };
    auto       pixels{ grey.samples | std::views::transform([&](std::uint8_t const index) { return drawn(index) ? index : std::uint8_t{ 0 }; }) | std::ranges::to<std::vector>() };
    if (std::ranges::all_of(grey.samples, drawn)) return Image{ grey.width, grey.height, std::move(pixels) };
    auto opaque{ grey.samples | std::views::transform([&](std::uint8_t const index) { return static_cast<std::uint8_t>(drawn(index)); }) | std::ranges::to<std::vector>() };
    return Image{ grey.width, grey.height, std::move(pixels), std::move(opaque) };
  });
}

}
