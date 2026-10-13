// One frame of the game's calls through services::Display, natively (the indexed Canvas, then VgalibRender for the
// pump) and through RenderDisplay on SDL's software renderer (then its present): the map view's 196 tiles, a dialog's
// box and two lines of text, a unit composed in a cell buffer, a cursor XORed on and off and a tracer drawn twice. At
// 60 Hz a frame has 16.7 ms; docs/rendering.md records both numbers.
#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/palette.hpp"
#include "opeu/assets/source.hpp"
#include "opeu/platform/sdl3/renderer-host.hpp"
#include "opeu/render/line-points.hpp"
#include "opeu/render/render-display.hpp"
#include "opeu/services/native-machine.hpp"
#include "opeu/services/vgalib.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <format>
#include <ranges>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace opeu::render::bench::display_frame {

[[nodiscard]] auto Here() -> std::filesystem::path {
  return std::filesystem::path{ std::source_location::current().file_name() }.parent_path();
}

// The shipped files and what the frame draws from them.
class Shipped {
public:
  Shipped() : _source{ assets::DirectorySource::Open(Here().parent_path().parent_path() / "opeu/assets.embed").value() }, _catalogue{ _source }, _index{ _catalogue } { }

  [[nodiscard]] auto Index() const noexcept -> assets::ContentIndex const& {
    return _index;
  }
  [[nodiscard]] auto Record(std::string_view const id, std::size_t const count) const -> std::vector<std::uint8_t> {
    auto const asset{ _catalogue.Find(id).value() };
    auto const file{ _catalogue.File(asset.file).value() };
    return file.subspan(asset.at, count) | std::views::transform([](std::byte const byte) { return std::to_integer<std::uint8_t>(byte); }) | std::ranges::to<std::vector>();
  }

private:
  assets::DirectorySource _source;
  assets::Catalogue       _catalogue;
  assets::ContentIndex    _index;
};

// The bytes of one frame's calls.
struct FrameBytes {
  std::vector<std::vector<std::uint8_t>> tiles;
  std::vector<std::vector<std::uint8_t>> glyphs;
  std::vector<std::uint8_t>              unit;
};

[[nodiscard]] auto BytesOf(Shipped const& shipped) -> FrameBytes {
  auto const             tile{ [&shipped](int const n) { return shipped.Record(std::format("view-tile/map-2/{}", n), 3uz * 24 * 4); } };
  auto const             glyph{ [&shipped](char const c) { return shipped.Record(std::format("text-glyph/{:03}", static_cast<int>(c)), 16); } };
  std::string_view const text{ "General, the bridgeat Arnhem is held." };
  return FrameBytes{ .tiles  = std::views::iota(10, 19) | std::views::transform(tile) | std::ranges::to<std::vector>(),
                     .glyphs = text | std::views::transform(glyph) | std::ranges::to<std::vector>(),
                     .unit   = shipped.Record("battle-unit/class-03/facing-1", 650) };
}

auto Map(services::Display& display, FrameBytes const& bytes) -> void {
  std::ranges::for_each(assets::Grid(14, 14), [&](auto const cell) {
    auto const& tile{ bytes.tiles.at((cell.first * 5 + cell.second * 3) % bytes.tiles.size()) };
    std::ignore = display.DrawColumns({ .x = static_cast<Word>(1 + cell.second * 3), .y = static_cast<Word>(16 + cell.first * 24), .width = 3, .height = 24 }, { .mode = 0, .bytes = tile });
  });
}

auto Dialog(services::Display& display, FrameBytes const& bytes) -> void {
  std::ignore = display.Box({ .x = 50, .y = 60, .width = 30, .height = 56 }, 1);
  std::ranges::for_each(std::views::zip(bytes.glyphs, std::views::iota(0)), [&display](auto const entry) {
    auto const& [glyph, n]{ entry };
    auto const at{
      services::Area{ .x = static_cast<Word>(52 + n % 19), .y = static_cast<Word>(64 + n / 19 * 16), .width = 1, .height = 16 }
    };
    display.DrawBitsOnPaper(at, { .mode = 0x000F, .bytes = glyph });
  });
}

auto Battle(services::Display& display, FrameBytes const& bytes) -> void {
  auto cell{ std::array<std::uint8_t, SYSLIB_CELL_BYTES>{} };
  display.SaveCell({ .x = 20, .y = 160 }, cell);
  display.MaskCell(cell, std::span{ bytes.unit }.subspan<SYSLIB_CELL_BYTES, services::CELL_MASK_BYTES>());
  display.OverlayCell(cell, std::span{ bytes.unit }.first<SYSLIB_CELL_BYTES>());
  display.RestoreShiftedCell({ .x = 20, .y = 160 }, cell);
  std::ranges::for_each(std::views::iota(0, 2), [&display](int) {
    display.Line(
        {
            .from = { .x = 80,  .y = 173 },
              .to = { .x = 300, .y = 90  }
    },
        0x001C);
    std::ignore = display.Highlight({ .x = 13, .y = 88, .width = 3, .height = 24 }, 0x010F);
  });
}

auto Frame(services::Display& display, FrameBytes const& bytes) -> void {
  Map(display, bytes);
  Dialog(display, bytes);
  Battle(display, bytes);
}

auto Start(services::Display& display) -> void {
  display.EnterGraphics();
  display.ShowIdentityPalette();
  display.LoadPalette(0, assets::FindPalette("main/day").value().levels);
}

auto NativeFrame(benchmark::State& state) -> void {
  auto const shipped{ Shipped{} };
  auto const bytes{ BytesOf(shipped) };
  vgalib_init();
  auto native{ services::NativeDisplay{} };
  Start(native);
  auto frame{ std::vector<std::uint8_t>(FRAME_PIXELS) };
  auto palette{ std::array<std::uint8_t, PALETTE_LEVELS>{} };
  for (auto _ : state) {
    Frame(native, bytes);
    benchmark::DoNotOptimize(VgalibRender(std::span<std::uint8_t, FRAME_PIXELS>{ frame.data(), FRAME_PIXELS }, palette));
  }
}
BENCHMARK(NativeFrame)->Unit(benchmark::kMicrosecond);

auto RenderedFrame(benchmark::State& state) -> void {
  auto const shipped{ Shipped{} };
  auto const bytes{ BytesOf(shipped) };
  auto       pack{ assets::OpenPack(Here().parent_path() / "render.test/packs/classic.opeupack") };
  if (!pack) throw std::runtime_error{ assets::Describe(pack.error()) };
  auto host{ platform::SurfaceRenderer::Open(LOGICAL_SIZE.width, LOGICAL_SIZE.height).value() };
  auto display{
    RenderDisplay{ Display::Open(host.Renderer()).value(), *pack, shipped.Index() }
  };
  Start(display);
  for (auto _ : state) {
    Frame(display, bytes);
    display.Present();
  }
  state.counters["recognised"] = static_cast<double>(display.Statistics().recognised);
  state.counters["unpacked"]   = static_cast<double>(display.Statistics().unpacked);
}
BENCHMARK(RenderedFrame)->Unit(benchmark::kMicrosecond);

// What the entry pays once: the content index over the shipped catalogue.
auto IndexShippedContent(benchmark::State& state) -> void {
  auto const source{ assets::DirectorySource::Open(Here().parent_path().parent_path() / "opeu/assets.embed").value() };
  auto const catalogue{ assets::Catalogue{ source } };
  for (auto _ : state) {
    benchmark::DoNotOptimize(assets::ContentIndex{ catalogue }.Size());
  }
}
BENCHMARK(IndexShippedContent)->Unit(benchmark::kMillisecond);

}
