// The cost of the renderer's operations on SDL's software renderer (the headless and test renderer; a window's GPU
// renderer queues the same calls). SDL queues drawing until a flush, so each drawing case flushes per iteration and
// measures the pixels actually drawn: a sprite blit, a scaled blit, a fill, a line of text, a snapshot and restore, a
// cache hit, the frame's present and readbacks. Compare with services.bench/drawing.cpp, the game's primitives today.
#include "opeu/assets/pack.hpp"
#include "opeu/platform/sdl3/renderer-host.hpp"
#include "opeu/render/atlas-text-renderer.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/texture-cache.hpp"

#include <benchmark/benchmark.h>

#include <filesystem>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <vector>

namespace opeu::render::bench {

// The tests' sample pack, beside this directory.
[[nodiscard]] auto SamplePack() -> assets::Pack {
  auto const here{ std::filesystem::path{ std::source_location::current().file_name() }.parent_path() };
  auto       pack{ assets::OpenPack(here.parent_path() / "render.test/packs/render-sample.opeupack") };
  if (!pack) throw std::runtime_error{ assets::Describe(pack.error()) };
  return *std::move(pack);
}

struct Bench {
  platform::SurfaceRenderer host{ platform::SurfaceRenderer::Open(LOGICAL_SIZE.width, LOGICAL_SIZE.height).value() };
  Display                   display{ Display::Open(host.Renderer()).value() };
  assets::Pack              pack{ SamplePack() };
  PackSource                source{ pack };
  TextureCache              textures{ host.Renderer(), source, 64 };

  [[nodiscard]] auto Unit() -> SharedBaked {
    return textures.Fetch(TextureKey{ .asset = "battle-unit/class-03/facing-1" }).value();
  }
};

auto BlitSprite(benchmark::State& state) -> void {
  auto       bench{ Bench{} };
  auto const unit{ bench.Unit() };
  bench.display.Begin();
  for (auto _ : state) {
    bench.display.Draw(unit->texture, Blit{ .destination = At(Point{ .x = 83, .y = 40 }, unit->size) });
    bench.display.Flush();
  }
  state.SetItemsProcessed(state.iterations() * 40 * 26);
}
BENCHMARK(BlitSprite);

auto BlitScaledFlipped(benchmark::State& state) -> void {
  auto       bench{ Bench{} };
  auto const unit{ bench.Unit() };
  auto const translucent{
    Rgba{ .r = 0xC0, .g = 0xC0, .b = 0xC0, .a = 0xC0 }
  };
  auto const blit{
    Blit{ .destination = Rect{ .x = 80, .y = 40, .width = 160, .height = 104 }, .flip = Flip::HORIZONTAL, .modulation = translucent }
  };
  bench.display.Begin();
  for (auto _ : state) {
    bench.display.Draw(unit->texture, blit);
    bench.display.Flush();
  }
  state.SetItemsProcessed(state.iterations() * 160 * 104);
}
BENCHMARK(BlitScaledFlipped);

auto FillRect(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) {
    bench.display.Fill(Rect{ .x = 10, .y = 50, .width = 320, .height = 100 }, Rgba{ .r = 0x10, .g = 0x41, .b = 0x92 });
    bench.display.Flush();
  }
  state.SetItemsProcessed(state.iterations() * 320 * 100);
}
BENCHMARK(FillRect);

auto TextLine(benchmark::State& state) -> void {
  auto       bench{ Bench{} };
  auto const text{ AtlasTextRenderer::Build(bench.host.Renderer(), bench.source, CLASSIC_FONTS).value() };
  auto const run{
    TextRun{ .font   = std::string{ GAME_FONT },
            .glyphs = std::views::iota(0u, 24u) | std::views::transform([](unsigned const n) { return n % 24; }) | std::ranges::to<std::vector<GlyphIndex>>(),
            .colour = WHITE,
            .at     = Point{ .x = 8, .y = 100 } }
  };
  bench.display.Begin();
  for (auto _ : state) {
    benchmark::DoNotOptimize(text.Draw(bench.display, run));
    bench.display.Flush();
  }
  state.SetItemsProcessed(state.iterations() * 24);
}
BENCHMARK(TextLine);

auto SnapshotRestore(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) {
    auto const snapshot{ bench.display.Take(Rect{ .x = 100, .y = 100, .width = 48, .height = 16 }).value() };
    bench.display.Restore(snapshot);
  }
}
BENCHMARK(SnapshotRestore);

auto CacheHit(benchmark::State& state) -> void {
  auto       bench{ Bench{} };
  auto const key{
    TextureKey{ .asset = "view-tile/map-2/17", .palette = "main/night" }
  };
  benchmark::DoNotOptimize(bench.textures.Fetch(key));
  for (auto _ : state) benchmark::DoNotOptimize(bench.textures.Fetch(key));
}
BENCHMARK(CacheHit);

auto CacheMissBake(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  auto textures{
    TextureCache{ bench.host.Renderer(), bench.source, 1 }
  };
  auto const day{
    TextureKey{ .asset = "view-tile/map-2/17", .palette = "main/day" }
  };
  auto const night{
    TextureKey{ .asset = "view-tile/map-2/17", .palette = "main/night" }
  };
  for (auto _ : state) {
    benchmark::DoNotOptimize(textures.Fetch(day));
    benchmark::DoNotOptimize(textures.Fetch(night));
  }
  state.SetItemsProcessed(state.iterations() * 2);
}
BENCHMARK(CacheMissBake);

auto Present(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) bench.display.Present(Dimmed(WHITE, 0xC0));
}
BENCHMARK(Present);

auto PresentUntinted(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) bench.display.Present();
}
BENCHMARK(PresentUntinted);

auto ReadbackFrame(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) benchmark::DoNotOptimize(bench.display.Read(LOGICAL_SCREEN));
}
BENCHMARK(ReadbackFrame);

auto ReadbackCell(benchmark::State& state) -> void {
  auto bench{ Bench{} };
  bench.display.Begin();
  for (auto _ : state) benchmark::DoNotOptimize(bench.display.Read(Rect{ .x = 20, .y = 100, .width = 32, .height = 32 }));
}
BENCHMARK(ReadbackCell);

}
