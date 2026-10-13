// The cost of the game's hottest drawing calls on the screen: column-image blits by mode, glyphs, battle cells,
// sprite composition, picture decoding and the frame handed to the platform.
#include "canvas.hpp"
#include "syslib-gfx.hpp"
#include "vgalib.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <format>
#include <numeric>
#include <ranges>
#include <span>
#include <string>

namespace opeu::services::bench {

// The benchmarks' buffers start on a cache line, as the canvas's pixel store does: an address that varies modulo 64
// between builds or processes (stack randomisation) would make the same copy straddle cache lines in one run and not
// in the next.
constexpr std::size_t CACHE_LINE{ 64 };

// 40 columns x 100 lines of a 4-byte-column image with every colour.
alignas(CACHE_LINE) auto const IMAGE{ [] {
  std::array<std::uint8_t, 40 * 100 * 4> image{};
  std::iota(image.begin(), image.end(), std::uint8_t{ 0 });
  return image;
}() };

auto Ready() -> void {
  vgalib_init();
  koei_2fea();
  koei_vga_identity_palette();
}

auto BlitColumns(benchmark::State& state) -> void {
  Ready();
  auto const mode{ static_cast<std::uint16_t>(state.range(0)) };
  for (auto _ : state) benchmark::DoNotOptimize(opeu_bitmap_restore(10, 50, 40, 100, mode, IMAGE.data()));
  state.SetItemsProcessed(state.iterations() * 320 * 100);
}
BENCHMARK(BlitColumns)->Args({ 0 })->Args({ 1 })->Args({ 4 })->ArgNames({ "mode" });

auto BlitThreeByteColumns(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) benchmark::DoNotOptimize(opeu_blit_far(10, 50, 40, 100, 4, IMAGE.data()));
  state.SetItemsProcessed(state.iterations() * 320 * 100);
}
BENCHMARK(BlitThreeByteColumns);

auto ShiftedGlyph(benchmark::State& state) -> void {
  Ready();
  alignas(CACHE_LINE) std::array<std::uint8_t, 32> glyph{};
  std::iota(glyph.begin(), glyph.end(), std::uint8_t{ 0x55 });
  for (auto _ : state) benchmark::DoNotOptimize(opeu_vga_glyph(101, 30, 16, 16, 0x000E, glyph.data()));
}
BENCHMARK(ShiftedGlyph);

auto AlignedGlyph(benchmark::State& state) -> void {
  Ready();
  alignas(CACHE_LINE) std::array<std::uint8_t, 32> glyph{};
  std::iota(glyph.begin(), glyph.end(), std::uint8_t{ 0x55 });
  for (auto _ : state) benchmark::DoNotOptimize(opeu_draw_far(12, 30, 2, 16, 0x0409, glyph.data()));
}
BENCHMARK(AlignedGlyph);

auto ShiftedTransparentImage(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) koei_2e57(83, 40, 5, 26, IMAGE.data());
}
BENCHMARK(ShiftedTransparentImage);

auto BattleCell(benchmark::State& state) -> void {
  Ready();
  alignas(CACHE_LINE) std::array<std::uint8_t, SYSLIB_CELL_BYTES> cell{};
  for (auto _ : state) {
    opeu_vram_save_odd(20, 100, cell.data());
    opeu_vram_restore_odd(20, 100, cell.data());
    opeu_vram_save(30, 100, cell.data());
    opeu_vram_restore(30, 100, cell.data());
  }
}
BENCHMARK(BattleCell);

// The block the benchmark copies: from game column 10, line 100 to column 20, line 140.
constexpr auto CELL_BLOCK_FROM{ static_cast<std::uintptr_t>(GameAt(10 * 8, 100)) };
constexpr auto CELL_BLOCK_TO{ static_cast<std::uintptr_t>(GameAt(20 * 8, 140)) };

// Where the copy's data and code landed in this process, for the perf gate's failure message: the pixel store's page
// offset, the source's and destination's, and the routine's offset in its 64-byte fetch block and in its page.
[[nodiscard]] auto CellBlockLayout() -> std::string {
  auto const store{ reinterpret_cast<std::uintptr_t>(Screen().Row(0).data()) }; // NOLINT(*-reinterpret-cast): the address itself is reported
  auto const code{ reinterpret_cast<std::uintptr_t>(&koei_3bc5) };              // NOLINT(*-reinterpret-cast): the address itself is reported
  return std::format("store%4096={} from%4096={} to%4096={} code%64={} code%4096={}", store % 4096, (store + CELL_BLOCK_FROM) % 4096, (store + CELL_BLOCK_TO) % 4096, code % 64, code % 4096);
}

auto CellBlockCopy(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) koei_3bc5(10, 100, 20, 140);
  state.SetLabel(CellBlockLayout());
}
BENCHMARK(CellBlockCopy);

auto SpriteCompose(benchmark::State& state) -> void {
  Ready();
  alignas(CACHE_LINE) std::array<std::uint8_t, 6 * 26 * 4> buffer{};
  for (auto _ : state) {
    opeu_sprite_compose(20, 100, 5, 26, IMAGE.data(), IMAGE.data() + 1024, buffer.data(), 1);
    koei_3f26(203, 100, 40, 26, IMAGE.data(), IMAGE.data() + 1024, buffer.data(), 1);
  }
}
BENCHMARK(SpriteCompose);

auto ScreenCopy(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) opeu_screen_copy(0, 240, 80, 48, 0, 320);
}
BENCHMARK(ScreenCopy);

// A full-screen plain picture: one literal and long copies per row.
alignas(CACHE_LINE) auto const PICTURE{ [] {
  std::array<std::uint8_t, 4 + 400 * 12> stream{ 0x80, 0x02, 0x90, 0x01 };
  for (auto const row : std::views::iota(0, 400)) {
    auto const commands{ std::span{ stream }.subspan(4 + row * 12uz, 12) };
    commands[0] = 0x05;
    commands[1] = static_cast<std::uint8_t>(row);
    std::ranges::fill(commands | std::views::drop(2), 0x8F);
  }
  return stream;
}() };
std::uint8_t const* picture_at;

auto PictureByte() -> std::uint8_t {
  return *picture_at++;
}

auto DecodePicture(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) {
    picture_at = PICTURE.data();
    benchmark::DoNotOptimize(opeu_picture_decode(0, 0, PictureByte));
  }
}
BENCHMARK(DecodePicture);

auto Present(benchmark::State& state) -> void {
  Ready();
  opeu_bitmap_restore(10, 50, 40, 100, 0, IMAGE.data());
  alignas(CACHE_LINE) static std::array<std::uint8_t, 640 * 480> frame{};
  alignas(CACHE_LINE) std::array<std::uint8_t, 768>              palette{};
  for (auto _ : state) {
    opeu_ui_box(0, 0, 1, 1, 3);
    benchmark::DoNotOptimize(VgalibRender(frame, palette));
    benchmark::ClobberMemory();
  }
}
BENCHMARK(Present);

}
