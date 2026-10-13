#pragma once
// The differential proof of RenderDisplay: the same script of services::Display calls through the native adapter
// (the indexed Canvas, observed as VgalibRender's frame in its palette, as the services tests observe it) and through
// RenderDisplay (its frame read back from the render target), the frames compared pixel by pixel in RGBA and the
// scripts' outcomes (return values, bytes saved into the game's memory) compared byte by byte.

#include "headless.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/content-index.hpp"
#include "opeu/assets/source.hpp"
#include "opeu/render/render-display.hpp"
#include "opeu/services/native-machine.hpp"
#include "opeu/services/vgalib.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <format>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace opeu::render::test {

// The shipped files, their catalogue and its content index, built once for every test of the run.
class ShippedAssets {
public:
  ShippedAssets() : _source{ assets::DirectorySource::Open("../opeu/assets.embed").value() }, _catalogue{ _source }, _index{ _catalogue } { }
  ShippedAssets(ShippedAssets const&)                    = delete;
  ShippedAssets(ShippedAssets&&)                         = delete;
  auto operator=(ShippedAssets const&) -> ShippedAssets& = delete;
  auto operator=(ShippedAssets&&) -> ShippedAssets&      = delete;
  ~ShippedAssets()                                       = default;

  [[nodiscard]] auto Index() const noexcept -> assets::ContentIndex const& {
    return _index;
  }
  // The bytes the game draws an asset from: its record in its file, `count` bytes of it.
  [[nodiscard]] auto Record(std::string_view const id, std::size_t const count) const -> std::vector<std::uint8_t> {
    auto const asset{ _catalogue.Find(id).value() };
    auto const file{ _catalogue.File(asset.file).value() };
    return file.subspan(asset.at, count) | std::views::transform([](std::byte const byte) { return std::to_integer<std::uint8_t>(byte); }) | std::ranges::to<std::vector>();
  }
  // A shipped file's bytes from `at` to its end.
  [[nodiscard]] auto FileFrom(std::string_view const name, std::size_t const at) const -> std::vector<std::uint8_t> {
    auto const file{ _catalogue.File(name).value() };
    return file.subspan(at) | std::views::transform([](std::byte const byte) { return std::to_integer<std::uint8_t>(byte); }) | std::ranges::to<std::vector>();
  }

private:
  assets::DirectorySource _source;
  assets::Catalogue       _catalogue;
  assets::ContentIndex    _index;
};

[[nodiscard]] inline auto Shipped() -> ShippedAssets const& {
  static auto const shipped{ std::make_unique<ShippedAssets const>() };
  return *shipped;
}

[[nodiscard]] inline auto ClassicSample() -> assets::Pack const& {
  static auto const pack{ std::make_unique<assets::Pack const>(OpenSample(CLASSIC)) };
  return *pack;
}

// What a script leaves besides the frame: the calls' results and the bytes it saved, in order.
using Outcome = std::vector<std::uint8_t>;
using Script  = std::function<Outcome(services::Display&)>;

struct Run {
  assets::Texture frame;
  Outcome         outcome;
};

// The native adapter's frame: the Canvas's palette entries in its palette's colours.
[[nodiscard]] inline auto NativeFrame() -> assets::Texture {
  auto frame{ std::vector<std::uint8_t>(FRAME_PIXELS) };
  auto palette{ std::array<std::uint8_t, PALETTE_LEVELS>{} };
  std::ignore = VgalibRender(std::span<std::uint8_t, FRAME_PIXELS>{ frame.data(), FRAME_PIXELS }, palette);
  auto const pixel{ [&palette](std::uint8_t const entry) {
    auto const at{ std::size_t{ entry } * 3 };
    return std::array{ std::byte{ palette.at(at) }, std::byte{ palette.at(at + 1) }, std::byte{ palette.at(at + 2) }, std::byte{ 0xFF } };
  } };
  return assets::Texture{ 640, 480, frame | std::views::transform(pixel) | std::views::join | std::ranges::to<std::vector>() };
}

// The script through the native adapter, from the start state.
[[nodiscard]] inline auto RunNative(Script const& script) -> Run {
  vgalib_init();
  auto native{ services::NativeDisplay{} };
  auto outcome{ script(native) };
  return Run{ .frame = NativeFrame(), .outcome = std::move(outcome) };
}

// The script through a fresh RenderDisplay on the software renderer over the classic sample.
class Rendered {
public:
  Rendered() : _host{ platform::SurfaceRenderer::Open(LOGICAL_SIZE.width, LOGICAL_SIZE.height).value() } {
    _display = std::make_unique<RenderDisplay>(Display::Open(_host.Renderer()).value(), ClassicSample(), Shipped().Index());
  }

  [[nodiscard]] auto Play(Script const& script) -> Run {
    auto outcome{ script(*_display) };
    return Run{ .frame = _display->Capture(), .outcome = std::move(outcome) };
  }
  [[nodiscard]] auto Screen() -> RenderDisplay& {
    return *_display;
  }

private:
  platform::SurfaceRenderer      _host;
  std::unique_ptr<RenderDisplay> _display;
};

// The pixels that differ, and the first of them said.
struct Difference {
  std::size_t pixels{};
  std::string first{};
};

[[nodiscard]] inline auto Compare(assets::Texture const& native, assets::Texture const& rendered) -> Difference {
  auto       difference{ Difference{} };
  auto const pixels{ std::views::zip(PixelsOf(native), PixelsOf(rendered), std::views::iota(0uz)) };
  std::ranges::for_each(pixels, [&difference](auto const entry) {
    auto const& [left, right, n]{ entry };
    if (std::ranges::equal(left, right)) return;
    if (difference.pixels++ == 0) {
      auto const colour{ [](auto const pixel) { return std::format("{:02X}{:02X}{:02X}", std::to_integer<int>(pixel[0]), std::to_integer<int>(pixel[1]), std::to_integer<int>(pixel[2])); } };
      difference.first = std::format("first at ({}, {}): native {} rendered {}", n % 640, n / 640, colour(left), colour(right));
    }
  });
  return difference;
}

// What the renderer's run left: its frame and how its images were found.
struct RenderedRun {
  assets::Texture frame;
  ByteStatistics  statistics;
};

// Both runs of the script; the frames and outcomes must be the same.
inline auto ExpectSame(Script const& script) -> RenderedRun {
  auto const native{ RunNative(script) };
  auto       rendered{ Rendered{} };
  auto       run{ rendered.Play(script) };
  auto const difference{ Compare(native.frame, run.frame) };
  EXPECT_EQ(difference.pixels, 0U) << difference.first;
  EXPECT_EQ(native.outcome, run.outcome);
  return RenderedRun{ .frame = std::move(run.frame), .statistics = rendered.Screen().Statistics() };
}

// Bytes of every value, from a seed: images in any format.
[[nodiscard]] inline auto Noise(std::size_t const count, std::uint32_t const seed) -> std::vector<std::uint8_t> {
  auto state{ seed };
  return std::views::iota(0uz, count) | std::views::transform([&state](std::size_t) {
           state = state * 1664525U + 1013904223U;
           return static_cast<std::uint8_t>(state >> 24);
         })
         | std::ranges::to<std::vector>();
}

// A picture stream as the game hands it over: a reader of its bytes, which counts what it was asked for (0 past the
// end).
class StreamBytes {
public:
  explicit StreamBytes(std::vector<std::uint8_t> bytes) : _bytes{ std::move(bytes) } { }

  [[nodiscard]] auto Reader() -> PictureByteReader {
    return [this] { return _read < _bytes.size() ? _bytes.at(_read++) : std::uint8_t{ 0 }; };
  }
  [[nodiscard]] auto Read() const noexcept -> std::size_t {
    return _read;
  }

private:
  std::vector<std::uint8_t> _bytes;
  std::size_t               _read{};
};

}
