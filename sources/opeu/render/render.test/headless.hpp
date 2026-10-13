#pragma once
// The tests' renderer: SDL's software renderer on a 640x480 surface (the same pixels on every machine), the sample
// pack in render.test/packs/ and the PNG goldens in render.test/goldens/. Tests run in the module directory.
//
// The sample is regenerated, when the pack format or the decoders change, with
//   python3 tools/assets/pack.py --out _local/render/sample --archive sources/opeu/render/render.test/packs/render-sample.opeupack --only <its ids>
// (the ids are its pack.json's), and the goldens with OPEU_REGENERATE_GOLDENS=1 on the test run.

#include "opeu/assets/pack.hpp"
#include "opeu/assets/png-reader.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/platform/sdl3/renderer-host.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/png-file.hpp"
#include "opeu/render/rows.hpp"

#include <SDL3/SDL.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::render::test {

inline constexpr auto SAMPLE_PACK{ std::string_view{ "render.test/packs/render-sample.opeupack" } };
inline constexpr auto GOLDENS{ std::string_view{ "render.test/goldens" } };

struct Headless {
  platform::SurfaceRenderer host;
  Display                   display;
};

[[nodiscard]] inline auto OpenHeadless() -> Headless {
  auto host{ platform::SurfaceRenderer::Open(LOGICAL_SIZE.width, LOGICAL_SIZE.height).value() };
  auto display{ Display::Open(host.Renderer()) };
  if (!display) throw std::runtime_error{ display.error().detail };
  return Headless{ .host = std::move(host), .display = *std::move(display) };
}

[[nodiscard]] inline auto OpenSample() -> assets::Pack {
  auto pack{ assets::OpenPack(SAMPLE_PACK) };
  if (!pack) throw std::runtime_error{ assets::Describe(pack.error()) };
  return *std::move(pack);
}

[[nodiscard]] inline auto Pixel(assets::Texture const& image, Point const at) -> Rgba {
  auto const offset{ ((static_cast<std::size_t>(at.y) * image.Width()) + static_cast<std::size_t>(at.x)) * 4 };
  auto const channel{ [&image, offset](std::size_t const n) { return std::to_integer<std::uint8_t>(image.Pixels().subspan(offset + n).front()); } };
  return Rgba{ .r = channel(0), .g = channel(1), .b = channel(2), .a = channel(3) };
}

// The top left `size` of an image.
[[nodiscard]] inline auto Crop(assets::Texture const& image, Size const size) -> assets::Texture {
  auto const layout{
    RowLayout{ .pitch = image.Pitch(), .width = static_cast<std::size_t>(size.width) * 4, .count = static_cast<std::size_t>(size.height) }
  };
  return assets::Texture{ static_cast<std::size_t>(size.width), static_cast<std::size_t>(size.height), Rows(image.Pixels(), layout) | std::views::join | std::ranges::to<std::vector>() };
}

// Every pixel, four bytes each.
[[nodiscard]] inline auto PixelsOf(assets::Texture const& image) {
  return Rows(image.Pixels(), RowLayout{ .pitch = 4, .width = 4, .count = image.Width() * image.Height() });
}

[[nodiscard]] inline auto ReadFile(std::filesystem::path const& path) -> std::vector<std::byte> {
  auto file{
    std::ifstream{ path, std::ios::binary }
  };
  return std::views::istream<char>(file >> std::noskipws) | std::views::transform([](char const c) { return static_cast<std::byte>(c); }) | std::ranges::to<std::vector>();
}

// The image equals render.test/goldens/<name>.png; OPEU_REGENERATE_GOLDENS=1 writes it instead.
inline auto ExpectGolden(assets::Texture const& image, std::string_view const name) -> void {
  auto const path{ std::filesystem::path{ GOLDENS } / std::format("{}.png", name) };
  if (std::getenv("OPEU_REGENERATE_GOLDENS") != nullptr) {
    auto const written{ WritePng(image, path) };
    ASSERT_TRUE(written) << written.error().detail;
    return;
  }
  auto const golden{ assets::ReadTexture(ReadFile(path)) };
  ASSERT_TRUE(golden) << path << ": " << assets::Describe(golden.error()) << " (OPEU_REGENERATE_GOLDENS=1 writes it)";
  ASSERT_EQ(golden->Width(), image.Width()) << path;
  ASSERT_EQ(golden->Height(), image.Height()) << path;
  auto const differing{ std::ranges::count_if(std::views::zip(golden->Pixels(), image.Pixels()), [](auto const pair) { return std::get<0>(pair) != std::get<1>(pair); }) };
  EXPECT_EQ(differing, 0) << path << ": " << differing << " bytes differ";
}

}
