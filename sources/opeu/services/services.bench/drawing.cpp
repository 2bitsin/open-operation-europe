// The cost of the game's hottest drawing calls on the screen: column-image blits by mode, glyphs, battle cells,
// sprite composition, picture decoding and the frame handed to the platform.
#include "syslib-gfx.hpp"
#include "vgalib.hpp"

#include <benchmark/benchmark.h>

#include <array>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <span>

namespace opeu::services::bench {

// 40 columns x 100 lines of a 4-byte-column image with every colour.
auto const IMAGE{ [] {
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
  std::array<std::uint8_t, 32> glyph{};
  std::iota(glyph.begin(), glyph.end(), std::uint8_t{ 0x55 });
  for (auto _ : state) benchmark::DoNotOptimize(opeu_vga_glyph(101, 30, 16, 16, 0x000E, glyph.data()));
}
BENCHMARK(ShiftedGlyph);

auto AlignedGlyph(benchmark::State& state) -> void {
  Ready();
  std::array<std::uint8_t, 32> glyph{};
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
  std::array<std::uint8_t, SYSLIB_CELL_BYTES> cell{};
  for (auto _ : state) {
    opeu_vram_save_odd(20, 100, cell.data());
    opeu_vram_restore_odd(20, 100, cell.data());
    opeu_vram_save(30, 100, cell.data());
    opeu_vram_restore(30, 100, cell.data());
  }
}
BENCHMARK(BattleCell);

auto CellBlockCopy(benchmark::State& state) -> void {
  Ready();
  for (auto _ : state) koei_3bc5(10, 100, 20, 140);
}
BENCHMARK(CellBlockCopy);

auto SpriteCompose(benchmark::State& state) -> void {
  Ready();
  std::array<std::uint8_t, 6 * 26 * 4> buffer{};
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
auto const PICTURE{ [] {
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
  static std::array<std::uint8_t, 640 * 480> frame{};
  std::array<std::uint8_t, 768>              palette{};
  for (auto _ : state) {
    opeu_ui_box(0, 0, 1, 1, 3);
    benchmark::DoNotOptimize(VgalibRender(frame.data(), palette.data()));
    benchmark::ClobberMemory();
  }
}
BENCHMARK(Present);

}
