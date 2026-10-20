#pragma once
// The tests' renderer: SDL's software renderer on a 640x480 surface (the same pixels on every machine), the packs
// they draw with (render.test/packs/<pack>.opeupack, a sample of each) and the PNG goldens: render.test/goldens/<name>.png
// for what is drawn without a pack, render.test/goldens/<pack>/<name>.png for what a pack's images make. A test that
// draws from a pack is parametrised by PackUnderTest, so a remaster pack is one more entry in PACKS and a directory of
// goldens. Tests run in the module directory.
//
// The classic sample is regenerated, when the pack format or the decoders change, with
//   python3 tools/assets/pack.py --out _local/render/sample --archive sources/opeu/render/render.test/packs/classic.opeupack --only <its ids>
// (the ids are its pack.json's; tools/assets/pack.test checks it is current), and the goldens with
// OPEU_REGENERATE_GOLDENS=1 on the test run, which writes each into its pack's directory.
//
// The samples and their goldens are made from the game's data, so they are the private repository's only: a suite that
// draws from a sample derives from SampleTest or ClassicSampleTest and skips, whole, in a clone without them.

#include "opeu/game-data.test.hpp"

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
#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iterator>
#include <optional>
#include <ostream>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::render::test {
using namespace std::string_literals;
using namespace std::string_view_literals;
inline constexpr auto GOLDENS{ "render.test/goldens"sv };

// A pack the tests draw with: its name, which names its goldens' directory, and its sample.
struct PackUnderTest {
  std::string_view name;
  std::string_view path;
};

inline constexpr auto CLASSIC{
  PackUnderTest{ .name = "classic", .path = "render.test/packs/classic.opeupack" }
};
inline constexpr auto PACKS{ std::array{ CLASSIC } };

// Skips the calling suite unless this clone holds every pack's sample.
inline auto SkipWithoutSamples() -> void {
  auto missing{ PACKS | std::views::filter([](PackUnderTest const& under_test) { return !::opeu::test::TestFile(under_test.path); }) };
  if (!std::ranges::empty(missing)) GTEST_SKIP() << ::opeu::test::NoGameData(std::format("the {} pack's sample ({})", missing.front().name, missing.front().path));
}

// A suite drawn from each pack's sample, PackUnderTest its parameter.
class SampleTest : public testing::TestWithParam<PackUnderTest> {
public:
  static auto SetUpTestSuite() -> void {
    SkipWithoutSamples();
  }
};

// A suite drawn from the classic sample alone.
class ClassicSampleTest : public testing::Test {
public:
  static auto SetUpTestSuite() -> void {
    SkipWithoutSamples();
  }
};

// gtest prints the parameter by its name.
inline auto operator<<(std::ostream& out, PackUnderTest const& under_test) -> std::ostream& {
  return out << under_test.name;
}

// The environment variable `name`, when set (SDL's copy of the environment, safe from any thread).
inline auto Environment(std::string const& name) -> std::optional<std::string> {
  auto const* const value{ SDL_getenv(name.c_str()) };
  return value != nullptr ? std::optional<std::string>{ value } : std::nullopt;
}

// What a test needs to go on: the value, or the test stops, saying what was missing.
template <class _Value>
[[nodiscard]] auto Needed(std::optional<_Value> found, std::string_view const what) -> _Value {
  if (!found) throw std::logic_error{ std::format("the test needs {}", what) };
  return *std::move(found);
}

// The parameter's name in a test's name: Scenes/DrawsEveryCommandInOrder/classic.
[[nodiscard]] inline auto PackName(testing::TestParamInfo<PackUnderTest> const& info) -> std::string {
  return std::string{ info.param.name };
}

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

[[nodiscard]] inline auto OpenSample(PackUnderTest const& under_test) -> assets::Pack {
  auto pack{ assets::OpenPack(under_test.path) };
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

// The image equals the golden at `path`; OPEU_REGENERATE_GOLDENS=1 writes it instead.
inline auto ExpectGoldenAt(assets::Texture const& image, std::filesystem::path const& path) -> void {
  if (Environment("OPEU_REGENERATE_GOLDENS")) {
    std::filesystem::create_directories(path.parent_path());
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

// What is drawn without a pack: render.test/goldens/<name>.png.
inline auto ExpectGolden(assets::Texture const& image, std::string_view const name) -> void {
  ExpectGoldenAt(image, std::filesystem::path{ GOLDENS } / std::format("{}.png", name));
}

// What a pack's images make: render.test/goldens/<pack>/<name>.png.
inline auto ExpectGolden(assets::Texture const& image, PackUnderTest const& under_test, std::string_view const name) -> void {
  ExpectGoldenAt(image, std::filesystem::path{ GOLDENS } / under_test.name / std::format("{}.png", name));
}

}
