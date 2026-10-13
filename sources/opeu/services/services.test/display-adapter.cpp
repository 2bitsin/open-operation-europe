// The native display forwards each method to the drawing function the game called: the same arguments give the same
// screen (or the same cell buffer) through the adapter as through the function.
#include "native-machine.hpp"

#include "canvas.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <ranges>
#include <span>
#include <vector>

namespace opeu::services {
namespace {

// Every byte value, as image data for any format.
auto const PATTERN{ [] {
  std::array<std::uint8_t, 2048> bytes{};
  std::iota(bytes.begin(), bytes.end(), std::uint8_t{ 7 });
  return bytes;
}() };

// A busy screen: every pixel a colour of its own place.
auto Paint() -> void {
  vgalib_init();
  koei_2fea();
  std::ranges::for_each(std::views::iota(0, 640 * 480), [](int const at) { Screen().Put(at, static_cast<std::uint8_t>(at * 7 / 5)); });
}

// The frame and palette after `draw` on the busy screen.
auto ScreenAfter(auto const& draw) -> std::vector<std::uint8_t> {
  Paint();
  draw();
  std::vector<std::uint8_t>     frame(640 * 480);
  std::array<std::uint8_t, 768> palette{};
  std::ignore = VgalibRender(std::span<std::uint8_t, 640 * 480>{ frame.data(), frame.size() }, palette);
  frame.insert(frame.end(), palette.begin(), palette.end());
  return frame;
}

class DisplayAdapter : public testing::Test {
protected:
  NativeDisplay _display;
  Image const   _image{ .mode = 0, .bytes = PATTERN };
};

TEST_F(DisplayAdapter, DrawsColumnsAsTheLibrary) {
  EXPECT_EQ(ScreenAfter([] { opeu_bitmap_restore(3, 20, 4, 9, 4, PATTERN.data()); }), ScreenAfter([&] { EXPECT_TRUE(_display.DrawColumns({ 3, 20, 4, 9 }, { 4, PATTERN })); }));
  EXPECT_EQ(ScreenAfter([] { opeu_blit_far(5, 30, 3, 8, 1, PATTERN.data()); }), ScreenAfter([&] { EXPECT_TRUE(_display.DrawThreeByteColumns({ 5, 30, 3, 8 }, { 1, PATTERN })); }));
  EXPECT_EQ(ScreenAfter([] { opeu_draw_far(2, 50, 6, 10, 0x0305, PATTERN.data()); }), ScreenAfter([&] { EXPECT_TRUE(_display.DrawBits({ 2, 50, 6, 10 }, { 0x0305, PATTERN })); }));
  EXPECT_EQ(ScreenAfter([] { opeu_code_0000_28c5(2, 50, 6, 10, 0x030C, PATTERN.data()); }), ScreenAfter([&] { _display.DrawBitsOnPaper({ 2, 50, 6, 10 }, { 0x030C, PATTERN }); }));
  EXPECT_EQ(ScreenAfter([] { opeu_gfx_blit(13, 60, 21, 7, 0x0104, PATTERN.data()); }), ScreenAfter([&] { EXPECT_TRUE(_display.DrawShiftedBits({ 13, 60, 21, 7 }, { 0x0104, PATTERN })); }));
  EXPECT_EQ(ScreenAfter([] { koei_2e57(37, 70, 3, 5, PATTERN.data()); }), ScreenAfter([&] { _display.DrawShiftedColumns({ 37, 70, 3, 5 }, PATTERN); }));
}

TEST_F(DisplayAdapter, ReportsWhatTheLibraryRejects) {
  EXPECT_FALSE(_display.DrawColumns({ 79, 0, 2, 1 }, _image));
  EXPECT_FALSE(_display.Box({ 80, 0, 1, 1 }, 1));
  EXPECT_FALSE(_display.Highlight({ 0, 0, 1, 1 }, 0x0003));
}

TEST_F(DisplayAdapter, SavesAndCopiesColumnsAsTheLibrary) {
  std::array<std::uint8_t, 4 * 4 * 6> direct{};
  std::array<std::uint8_t, 4 * 4 * 6> adapted{};
  Paint();
  opeu_bitmap_save(9, 33, 4, 6, direct.data());
  _display.SaveColumns({ 9, 33, 4, 6 }, adapted);
  EXPECT_EQ(direct, adapted);
  EXPECT_EQ(ScreenAfter([] { opeu_screen_copy(4, 10, 6, 12, 9, 14); }), ScreenAfter([&] { _display.CopyColumns({ 4, 10, 6, 12 }, { 9, 14 }); }));
}

TEST_F(DisplayAdapter, DrawsShapesAsTheLibrary) {
  EXPECT_EQ(ScreenAfter([] { opeu_ui_box(2, 3, 5, 7, 0x0B); }), ScreenAfter([&] { EXPECT_TRUE(_display.Box({ 2, 3, 5, 7 }, 0x0B)); }));
  EXPECT_EQ(ScreenAfter([] { opeu_ui_highlight(2, 3, 5, 7, 0x0103); }), ScreenAfter([&] { EXPECT_TRUE(_display.Highlight({ 2, 3, 5, 7 }, 0x0103)); }));
  EXPECT_EQ(ScreenAfter([] { opeu_ui_line(10, 20, 300, 170, 0xAA13); }), ScreenAfter([&] {
              _display.Line(
                  {
                      { 10,  20  },
                      { 300, 170 }
              },
                  0xAA13);
            }));
  EXPECT_EQ(ScreenAfter([] { opeu_ui_fill(5, 10, 221, 31, 6); }), ScreenAfter([&] {
              _display.Fill(
                  {
                      { 5,   10 },
                      { 221, 31 }
              },
                  6);
            }));
}

TEST_F(DisplayAdapter, KeepsTheScreenAndPaletteAsTheLibrary) {
  EXPECT_EQ(ScreenAfter([] { opeu_vga_clear(); }), ScreenAfter([&] { _display.Clear(); }));
  EXPECT_EQ(ScreenAfter([] { opeu_set_palette(3, 5, PATTERN.data()); }), ScreenAfter([&] { _display.LoadPalette(3, std::span{ PATTERN }.first(15)); }));
  EXPECT_EQ(ScreenAfter([] { koei_vga_identity_palette(); }), ScreenAfter([&] { _display.ShowIdentityPalette(); }));
  EXPECT_EQ(ScreenAfter([] { opeu_text_mode(); }), ScreenAfter([&] { _display.LeaveGraphics(); }));
  EXPECT_EQ(ScreenAfter([] {
              opeu_text_mode();
              koei_2fea();
            }),
            ScreenAfter([&] {
              _display.LeaveGraphics();
              _display.EnterGraphics();
            }));
}

TEST_F(DisplayAdapter, DecodesPicturesAsTheLibrary) {
  static constexpr std::array<std::uint8_t, 7> PICTURE{ 8, 0, 2, 0, 0x14, 0x21, 0xC1 };
  auto const                                   reader{ [] { return [read = 0uz] mutable { return PICTURE[read++]; }; } };
  EXPECT_EQ(ScreenAfter([&] { opeu_picture_decode(8, 3, reader()); }), ScreenAfter([&] { EXPECT_TRUE(_display.DecodePicture({ 8, 3 }, reader())); }));
  EXPECT_EQ(ScreenAfter([&] { opeu_picture_decode_masked(8, 3, reader()); }), ScreenAfter([&] { std::ignore = _display.DecodeMaskedPicture({ 8, 3 }, reader()); }));
}

TEST_F(DisplayAdapter, MovesBattleCellsAsTheLibrary) {
  std::array<std::uint8_t, SYSLIB_CELL_BYTES> cell{};
  std::ranges::copy(std::span{ PATTERN }.first(cell.size()), cell.begin());
  EXPECT_EQ(ScreenAfter([&] { opeu_vram_restore(10, 100, cell.data()); }), ScreenAfter([&] { _display.RestoreCell({ 10, 100 }, cell); }));
  EXPECT_EQ(ScreenAfter([&] { opeu_vram_restore_odd(20, 50, cell.data()); }), ScreenAfter([&] { _display.RestoreShiftedCell({ 20, 50 }, cell); }));
  EXPECT_EQ(ScreenAfter([] { koei_3bc5(10, 100, 20, 140); }), ScreenAfter([&] { _display.CopyCellBlock({ 10, 100 }, { 20, 140 }); }));
  std::array<std::uint8_t, SYSLIB_CELL_BYTES> direct{};
  std::array<std::uint8_t, SYSLIB_CELL_BYTES> adapted{};
  Paint();
  opeu_vram_save(7, 90, direct.data());
  _display.SaveCell({ 7, 90 }, adapted);
  EXPECT_EQ(direct, adapted);
  opeu_vram_save_odd(7, 90, direct.data());
  _display.SaveShiftedCell({ 7, 90 }, adapted);
  EXPECT_EQ(direct, adapted);
}

TEST_F(DisplayAdapter, ComposesCellBuffersAsTheLibrary) {
  std::array<std::uint8_t, SYSLIB_CELL_BYTES> direct{};
  std::ranges::copy(std::span{ PATTERN }.subspan(100, direct.size()), direct.begin());
  auto            adapted{ direct };
  CellImage const image{ std::span{ PATTERN }.subspan<300, SYSLIB_CELL_BYTES>() };
  CellMask const  mask{ std::span{ PATTERN }.subspan<900, CELL_MASK_BYTES>() };
  CellGlyph const glyph{ std::span{ PATTERN }.subspan<1200, CELL_GLYPH_LINES>() };
  opeu_vram_draw(direct.data(), image.data());
  _display.OverlayCell(adapted, image);
  opeu_vram_load(direct.data(), mask.data());
  _display.MaskCell(adapted, mask);
  opeu_vram_glyph(direct.data(), glyph.data(), 17, 8);
  _display.DrawCellGlyph(adapted, glyph, { 17, 8 });
  opeu_vram_glyph_color(direct.data(), glyph.data(), 11, 13, 0x0100);
  _display.DrawColouredCellGlyph(adapted, glyph,
                                 {
                                     .at = { 11, 13 },
                                       .colour = 0x0100
  });
  EXPECT_EQ(direct, adapted);
}

TEST_F(DisplayAdapter, ComposesSpritesAsTheLibrary) {
  std::array<std::uint8_t, 6 * 26 * 4> direct{};
  std::array<std::uint8_t, 6 * 26 * 4> adapted{};
  auto const                           sprite{ [&](std::span<std::uint8_t> const buffer) {
    return Sprite{ .and_mask = std::span{ PATTERN }.first(1024), .or_image = std::span{ PATTERN }.subspan(1024), .buffer = buffer, .flags = 1 };
  } };
  EXPECT_EQ(ScreenAfter([&] { opeu_sprite_compose(20, 100, 5, 26, PATTERN.data(), std::span{ PATTERN }.subspan(1024).data(), direct.data(), 1); }),
            ScreenAfter([&] { _display.ComposeSprite({ 20, 100, 5, 26 }, sprite(adapted)); }));
  EXPECT_EQ(direct, adapted);
  EXPECT_EQ(ScreenAfter([&] { koei_3f26(203, 100, 40, 26, PATTERN.data(), std::span{ PATTERN }.subspan(1024).data(), direct.data(), 1); }),
            ScreenAfter([&] { _display.ComposeShiftedSprite({ 203, 100, 40, 26 }, sprite(adapted)); }));
  EXPECT_EQ(direct, adapted);
}

}
}
