// The cost of the Canvas primitives themselves, without the game library's wrappers: column images in every plane
// count and mode, 1-bit glyphs on 8-pixel boundaries and shifted, saving columns back, boxes, highlights, fills,
// lines, column copies and the rendering of the whole frame.
#include "canvas.hpp"
#include "calibration.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <span>

namespace opeu::services::bench {
namespace {

inline constexpr auto COLUMNS{ 40 }; // a 320 x 100 image
inline constexpr auto LINES{ 100 };
inline constexpr auto GLYPH_BYTES{ 32 }; // 16 x 16 pixels, 1 bit each

// Bytes counting up through every value, so every colour and every bit pattern occurs.
template <std::size_t SIZE>
[[nodiscard]] constexpr auto Counting(std::uint8_t first) -> std::array<std::uint8_t, SIZE> {
  std::array<std::uint8_t, SIZE> bytes{};
  std::iota(bytes.begin(), bytes.end(), first);
  return bytes;
}

inline constexpr auto IMAGE{ Counting<COLUMNS * LINES * 4>(0) };
inline constexpr auto GLYPH{ Counting<GLYPH_BYTES>(0x55) };

inline constexpr auto PIXELS{ detail::canvas::PIXELS };
inline constexpr auto PALETTE_BYTES{ detail::canvas::PALETTE_BYTES };

// A started canvas in graphics mode, owned for one benchmark (it is too large for the stack).
[[nodiscard]] auto StartedCanvas() -> std::unique_ptr<Canvas> {
  auto canvas{ std::make_unique<Canvas>() };
  canvas->Start();
  canvas->EnterGraphics();
  return canvas;
}

// A benchmark of a call that draws nothing measures nothing: it stops with an error instead.
auto RequireDrawn(benchmark::State& state, Drawn const drawn) -> void {
  if (!drawn) state.SkipWithError("the call drew nothing");
}

auto DrawColumns(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  auto const mode{ static_cast<unsigned>(state.range(0)) };
  auto const planes{ static_cast<int>(state.range(1)) };
  auto const image{ std::span{ IMAGE }.first(static_cast<std::size_t>(COLUMNS * LINES * planes)) };
  RequireDrawn(state, board->DrawColumns(10, 50, COLUMNS, LINES, mode, image, planes));
  for (auto _ : state) benchmark::DoNotOptimize(board->DrawColumns(10, 50, COLUMNS, LINES, mode, image, planes));
  state.SetItemsProcessed(state.iterations() * COLUMNS * 8 * LINES);
}
BENCHMARK(DrawColumns)->Args({ 0, 4 })->Args({ 1, 4 })->Args({ 4, 4 })->Args({ 0, 3 })->ArgNames({ "mode", "planes" });

auto SaveColumns(benchmark::State& state) -> void {
  auto const                                    board{ StartedCanvas() };
  std::array<std::uint8_t, COLUMNS * LINES * 4> saved{};
  for (auto _ : state) {
    board->SaveColumns(10, 50, COLUMNS, LINES, saved);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * COLUMNS * 8 * LINES);
}
BENCHMARK(SaveColumns);

auto DrawShiftedColumns(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  for (auto _ : state) {
    board->DrawShiftedColumns(83, 40, 5, 26, IMAGE);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(DrawShiftedColumns);

auto AlignedGlyph(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  RequireDrawn(state, board->DrawBits(12, 30, 2, 16, 0x0409, GLYPH));
  for (auto _ : state) benchmark::DoNotOptimize(board->DrawBits(12, 30, 2, 16, 0x0409, GLYPH));
}
BENCHMARK(AlignedGlyph);

auto ShiftedGlyph(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  RequireDrawn(state, board->DrawShiftedBits(101, 30, 16, 16, 0x000E, GLYPH));
  for (auto _ : state) benchmark::DoNotOptimize(board->DrawShiftedBits(101, 30, 16, 16, 0x000E, GLYPH));
}
BENCHMARK(ShiftedGlyph);

auto BoxAndHighlight(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  RequireDrawn(state, board->Box(2, 100, 60, 60, 3));
  RequireDrawn(state, board->Highlight(2, 100, 60, 60, 0x0105));
  for (auto _ : state) {
    benchmark::DoNotOptimize(board->Box(2, 100, 60, 60, 3));            // columns 2..61, 60 lines
    benchmark::DoNotOptimize(board->Highlight(2, 100, 60, 60, 0x0105)); // XOR colour 5
  }
}
BENCHMARK(BoxAndHighlight);

auto Fill(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  for (auto _ : state) {
    board->Fill(0, 0, 639, 399, 6);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * 640 * 400);
}
BENCHMARK(Fill);

auto Lines(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  for (auto _ : state) {
    board->Line(0, 0, 639, 399, 7);
    board->Line(639, 0, 0, 399, 9);
    board->Line(0, 200, 639, 200, 11);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(Lines);

auto CopyColumns(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  for (auto _ : state) {
    board->CopyColumns(0, 240, 80, 48, 0, 320);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(CopyColumns);

auto Render(benchmark::State& state) -> void {
  auto const board{ StartedCanvas() };
  benchmark::DoNotOptimize(board->DrawColumns(10, 50, COLUMNS, LINES, 0, IMAGE, 4));
  auto                                    frame{ std::make_unique<std::array<std::uint8_t, PIXELS>>() };
  std::array<std::uint8_t, PALETTE_BYTES> palette{};
  for (auto _ : state) {
    board->Render(*frame, palette);
    benchmark::ClobberMemory();
  }
  state.SetItemsProcessed(state.iterations() * PIXELS);
}
BENCHMARK(Render);

} // namespace
} // namespace opeu::services::bench
