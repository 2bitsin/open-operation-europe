// Checks of the drawing library on the canvas against results worked out by hand.
#include "vgalib.hpp"

#include "canvas.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

namespace opeu::services {
namespace {

// The colour at game coordinates (x, y) (game line y = screen line y + 40).
auto Px(int const x, int const y) -> int {
  return Screen().Pixel(GameAt(x, y));
}

auto SetPx(int const x, int const y, int const colour) -> void {
  Screen().Put(GameAt(x, y), static_cast<std::uint8_t>(colour));
}

auto FillPx(int const colour) -> void {
  std::ranges::for_each(std::views::iota(0, 640 * 480), [colour](int const at) { Screen().Put(at, static_cast<std::uint8_t>(colour)); });
}

// Expects `colour(x)` at x of game line y for every x in [from, to).
auto ExpectRow(int const y, std::pair<int, int> const columns, auto const& colour) -> void {
  std::ranges::for_each(std::views::iota(columns.first, columns.second), [&](int const x) { EXPECT_EQ(Px(x, y), colour(x)) << "x=" << x << " y=" << y; });
}

// A picture stream read through an injected reader; Consumed() tells how far the decoder read.
class Stream {
public:
  explicit Stream(std::span<std::uint8_t const> const bytes) : _bytes{ bytes } { }

  [[nodiscard]] auto Reader() -> PictureByteReader {
    return [this] { return _bytes[_read++]; };
  }
  [[nodiscard]] auto Consumed() const -> std::size_t {
    return _read;
  }

private:
  std::span<std::uint8_t const> _bytes;
  std::size_t                   _read{};
};

class Vgalib : public testing::Test {
protected:
  auto SetUp() -> void override {
    vgalib_init();
    koei_2fea();
  }
};

// 2 columns x 2 rows; column bytes are planes 0..3.
constexpr std::array<std::uint8_t, 16> IMAGE_4{
  0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, // row 0: colour 1, colour 2
  0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x00, 0x00, 0x0F, // row 1: 15 x4 + 0, 0 x4 + 9
};

TEST_F(Vgalib, RestoresAndSavesFourByteColumns) {
  std::array<std::uint8_t, 16> back{};
  FillPx(0);
  EXPECT_EQ(opeu_bitmap_restore(10, 5, 2, 2, 0, IMAGE_4.data()), 0);
  ExpectRow(5, { 80, 88 }, [](int) { return 1; });
  ExpectRow(5, { 88, 96 }, [](int) { return 2; });
  EXPECT_TRUE(Px(80, 6) == 15 && Px(84, 6) == 0) << "row 1 column 0 nibbles";
  EXPECT_TRUE(Px(88, 6) == 0 && Px(92, 6) == 9) << "row 1 column 1 nibbles";
  opeu_bitmap_save(10, 5, 2, 2, back.data());
  EXPECT_EQ(back, IMAGE_4) << "save round trip";
}

TEST_F(Vgalib, RejectsFourByteColumnsOutsideTheGameArea) {
  EXPECT_EQ(opeu_bitmap_restore(79, 0, 2, 1, 0, IMAGE_4.data()), -1) << "x + w > 80";
  EXPECT_EQ(opeu_bitmap_restore(0, 399, 1, 2, 0, IMAGE_4.data()), -1) << "y + h > 400";
  EXPECT_EQ(opeu_bitmap_restore(0, 0, 1, 1, 5, IMAGE_4.data()), -1) << "mode 5";
}

TEST_F(Vgalib, DrawsFourByteColumnsInEveryMode) {
  // Row doubling (bit 15): 2 source rows -> 4 lines; w = 1: the rows are bytes 0..3 (colour 1) and 4..7 (colour 2).
  FillPx(0);
  opeu_bitmap_restore(0, 0, 1, 2, 0x8000, IMAGE_4.data());
  EXPECT_TRUE(Px(0, 0) == 1 && Px(0, 1) == 1 && Px(0, 2) == 2 && Px(0, 3) == 2 && Px(0, 4) == 0) << "doubled rows";
  // Transparent mode 4: zero pixels keep the background.
  FillPx(6);
  opeu_bitmap_restore(0, 0, 2, 2, 4, IMAGE_4.data());
  EXPECT_TRUE(Px(0, 1) == 15 && Px(4, 1) == 6) << "mode 4 opaque/transparent";
  EXPECT_TRUE(Px(8, 1) == 6 && Px(12, 1) == 9) << "mode 4 second column";
  FillPx(3);
  koei_266c(0, 0, 1, 1, 1, IMAGE_4.data());
  EXPECT_EQ(Px(0, 0), 3 ^ 1) << "xor";
  FillPx(3);
  koei_266c(0, 0, 1, 1, 2, std::span{ IMAGE_4 }.subspan(4).data()); // colour 2
  EXPECT_EQ(Px(0, 0), 3 & 2) << "and";
  FillPx(4);
  koei_266c(0, 0, 1, 1, 3, std::span{ IMAGE_4 }.subspan(4).data());
  EXPECT_EQ(Px(0, 0), 4 | 2) << "or";
}

TEST_F(Vgalib, DrawsThreeByteColumns) {
  constexpr std::array<std::uint8_t, 3> IMAGE{ 0xFF, 0x0F, 0x00 };
  constexpr std::array<std::uint8_t, 3> MASK{ 0xF0, 0x00, 0x00 };
  FillPx(8);
  opeu_blit_far(0, 0, 1, 1, 0, IMAGE.data());
  EXPECT_TRUE(Px(0, 0) == 1 && Px(4, 0) == 3) << "plane 3 cleared";
  FillPx(0);
  koei_27c2(0, 0, 1, 1, 0x4000, IMAGE.data());
  EXPECT_TRUE(Px(0, 0) == 9 && Px(4, 0) == 11) << "plane 3 set by bit 14";
  FillPx(12);                                 // planes 2, 3
  opeu_blit_far(0, 0, 1, 1, 1, IMAGE.data()); // XOR planes 0-2 only
  EXPECT_TRUE(Px(0, 0) == (12 ^ 1) && Px(4, 0) == (12 ^ 3)) << "xor keeps plane 3";
  FillPx(14);
  opeu_blit_far(0, 0, 1, 1, 4, MASK.data());
  EXPECT_TRUE(Px(0, 0) == 1 && Px(4, 0) == 14) << "mode 4 clears plane 3 under the mask";
}

TEST_F(Vgalib, DrawsOneBitImages) {
  constexpr std::array<std::uint8_t, 2> BITS{ 0xA0, 0x01 };
  FillPx(7);
  opeu_draw_near(1, 2, 2, 1, 0x0005, BITS.data()); // opaque, colour 5
  EXPECT_TRUE(Px(8, 2) == 5 && Px(9, 2) == 0 && Px(10, 2) == 5 && Px(11, 2) == 0) << "opaque";
  EXPECT_TRUE(Px(23, 2) == 5 && Px(22, 2) == 0) << "opaque byte 2";
  FillPx(7);
  opeu_draw_far(1, 2, 1, 1, 0x0409, BITS.data()); // op 4 = set, colour 9
  EXPECT_TRUE(Px(8, 2) == 9 && Px(9, 2) == 7) << "transparent set";
  FillPx(6);
  opeu_draw_far(1, 2, 1, 1, 0x0103, BITS.data()); // XOR 3
  EXPECT_TRUE(Px(8, 2) == (6 ^ 3) && Px(9, 2) == 6) << "xor";
  FillPx(6);
  opeu_draw_far(1, 2, 1, 1, 0x0202, BITS.data()); // AND: 0 bits clear colour planes
  EXPECT_TRUE(Px(8, 2) == 6 && Px(9, 2) == (6 & ~2)) << "and";
  FillPx(4);
  opeu_draw_far(1, 2, 1, 1, 0x0301, BITS.data()); // OR
  EXPECT_TRUE(Px(8, 2) == 5 && Px(9, 2) == 4) << "or";
  // Operation 7 keeps the previous selection (OR 1) and ignores colour 8.
  FillPx(4);
  opeu_draw_far(1, 2, 1, 1, 0x0708, BITS.data());
  EXPECT_TRUE(Px(8, 2) == 5 && Px(9, 2) == 4) << "op 7 keeps the previous writer state";
  FillPx(0);
  opeu_draw_far(0, 0, 1, 1, 0x800F, BITS.data());
  EXPECT_TRUE(Px(0, 0) == 15 && Px(0, 1) == 15 && Px(1, 1) == 0) << "doubled 1-bit";
  // 28C5: foreground DL, paper DH.
  FillPx(7);
  opeu_code_0000_28c5(0, 0, 1, 1, 0x030C, BITS.data());
  EXPECT_TRUE(Px(0, 0) == 12 && Px(1, 0) == 3 && Px(2, 0) == 12 && Px(7, 0) == 3) << "28c5 two colours";
}

TEST_F(Vgalib, DrawsShiftedGlyphs) {
  // 10 pixels wide: 2 source bytes per row; x = 3.
  constexpr std::array<std::uint8_t, 2> GLYPH{ 0xFF, 0xC0 };
  FillPx(1);
  EXPECT_EQ(opeu_vga_glyph(3, 0, 10, 1, 0x000E, GLYPH.data()), 0);
  ExpectRow(0, { 0, 24 }, [](int const x) { return x >= 3 && x < 13 ? 14 : 1; });
  // AND: 1 bits and the FF fill keep pixels; the zero bits of the last source byte (glyph pixels 10..15 -> x 13..18)
  // clear plane 2.
  FillPx(15);
  opeu_gfx_blit(3, 0, 10, 1, 0x0204, GLYPH.data());
  ExpectRow(0, { 0, 24 }, [](int const x) { return x >= 13 && x <= 18 ? 11 : 15; });
  EXPECT_EQ(opeu_vga_glyph(635, 0, 10, 1, 0, GLYPH.data()), -1) << "glyph clip";
}

TEST_F(Vgalib, FillsBoxesAndHighlights) {
  FillPx(0);
  EXPECT_EQ(opeu_ui_box(2, 3, 2, 2, 0x0B), 0);
  EXPECT_TRUE(Px(15, 3) == 0 && Px(16, 3) == 11 && Px(31, 4) == 11 && Px(32, 4) == 0) << "box extent";
  EXPECT_EQ(opeu_vga_fill_rect(80, 0, 1, 1, 1), -1) << "box clip";
  FillPx(0);
  opeu_ui_fill(5, 10, 21, 11, 6);
  ExpectRow(10, { 0, 30 }, [](int const x) { return x >= 5 && x <= 21 ? 6 : 0; });
  EXPECT_TRUE(Px(5, 12) == 0 && Px(5, 11) == 6 && Px(5, 9) == 0) << "fill rows";
  FillPx(0);
  opeu_ui_fill(2, 0, 4, 0, 3); // single byte
  ExpectRow(0, { 0, 16 }, [](int const x) { return x >= 2 && x <= 4 ? 3 : 0; });
  FillPx(5);
  EXPECT_EQ(opeu_ui_highlight(0, 0, 1, 1, 0x0103), 0) << "highlight xor";
  EXPECT_EQ(Px(0, 0), 6) << "highlight xor result";
  EXPECT_EQ(opeu_ui_highlight(0, 0, 1, 1, 0x0003), -1) << "highlight op 0 rejected";
  FillPx(5);
  opeu_ui_highlight(0, 0, 1, 1, 0x0205);
  EXPECT_EQ(Px(0, 0), 5) << "highlight AND all bits: no change";
  FillPx(9);
  opeu_vga_clear();
  EXPECT_TRUE(Px(0, -40) == 0 && Px(639, 439) == 0) << "clear visible";
}

TEST_F(Vgalib, DrawsLines) {
  FillPx(0);
  opeu_ui_line(0, 0, 10, 0, 0x0004);
  ExpectRow(0, { 0, 12 }, [](int const x) { return x <= 10 ? 4 : 0; });
  FillPx(0);
  opeu_ui_line(3, 9, 3, 0, 0x0002); // vertical, reversed
  std::ranges::for_each(std::views::iota(0, 11), [](int const y) { EXPECT_EQ(Px(3, y), y <= 9 ? 2 : 0) << "vline " << y; });
  FillPx(0);
  opeu_ui_line(0, 0, 4, 4, 0x0001);
  std::ranges::for_each(std::views::iota(0, 5), [](int const x) { EXPECT_EQ(Px(x, x), 1) << "diagonal " << x; });
  FillPx(0);
  opeu_ui_line(0, 4, 4, 0, 0x0001); // rising: step -1
  std::ranges::for_each(std::views::iota(0, 5), [](int const x) { EXPECT_EQ(Px(x, 4 - x), 1) << "antidiagonal " << x; });
  // Shallow, 0,0 -> 4,2 (di = 2, dy = 2, dx = 4): (0,0) di = 0; (1,0) borrow -> di = 2, y = 1; (2,1) di = 0; (3,1)
  // borrow -> y = 2; (4,2).
  FillPx(0);
  opeu_ui_line(0, 0, 4, 2, 0x0001);
  EXPECT_TRUE(Px(0, 0) == 1 && Px(1, 0) == 1 && Px(2, 1) == 1 && Px(3, 1) == 1 && Px(4, 2) == 1) << "bresenham sequence";
  EXPECT_TRUE(Px(2, 0) == 0 && Px(4, 1) == 0) << "bresenham off-points";
  // Dash pattern AAh with mode 0: the off pixels drawn in colour 0.
  FillPx(7);
  opeu_ui_line(0, 0, 7, 0, 0xAA03);
  ExpectRow(0, { 0, 8 }, [](int const x) { return (x & 1) != 0 ? 0 : 3; });
  // Mode 4 (pixel |= colour) only on pattern bits.
  FillPx(8);
  opeu_ui_line(0, 0, 7, 0, 0xF041);
  ExpectRow(0, { 0, 8 }, [](int const x) { return x < 4 ? 9 : 8; });
  FillPx(5);
  opeu_ui_line(0, 0, 2, 0, 0x0013);
  EXPECT_TRUE(Px(0, 0) == 6 && Px(3, 0) == 5) << "xor line";
  FillPx(0);
  Koei2ce4(639, 399, 0x0F);
  Koei2ce4(640, 0, 0x0F);
  EXPECT_EQ(Px(639, 399), 15) << "plot";
  EXPECT_EQ(Px(0, 1), 0) << "plot clipped";
}

TEST_F(Vgalib, KeepsTheShiftedColumnsLatchQuirk) {
  // 2 source columns, shift 4. Column 0 all colour 1 except a transparent right half; column 1 a transparent left half.
  constexpr std::array<std::uint8_t, 8> IMAGE{ 0xF0, 0, 0, 0, 0x0F, 0, 0, 0 };
  FillPx(0);
  std::ranges::for_each(std::views::iota(0, 24), [](int const x) { SetPx(x, 0, x < 8 ? 2 : (x < 16 ? 4 : 8)); });
  Koei2e49(4, 0, 2, 1, IMAGE.data());
  // Byte 0 (x 0..7): mask 0F -> x 4..7 colour 1, x 0..3 keep 2. Byte 1 (x 8..15): data (F0 0F) >> 4 = 0 | mask
  // (F0 << 8 | 0F) >> 4 = 0 -> every bit from the latch of byte 0 (colour 2), not its own colour 4. The final partial
  // byte (x 16..23): mask 0F << 4 = F0 with a fresh latch: x 16..19 colour 1, x 20..23 keep 8.
  ExpectRow(0, { 0, 24 }, [](int const x) { return x < 4 ? 2 : x < 8 ? 1 : x < 16 ? 2 : x < 20 ? 1 : 8; });
}

TEST_F(Vgalib, CopiesColumnsAsMemmove) {
  auto const three_bytes{ [] { std::ranges::for_each(std::views::iota(0, 24), [](int const x) { SetPx(x, 0, (x / 8) + 1); }); } };
  FillPx(0);
  three_bytes();
  opeu_screen_copy(0, 0, 2, 1, 1, 0); // overlapping, destination right
  EXPECT_TRUE(Px(0, 0) == 1 && Px(8, 0) == 1 && Px(16, 0) == 2) << "backward copy";
  FillPx(0);
  three_bytes();
  opeu_vga_copy(1, 0, 2, 1, 0, 0); // overlapping, destination left
  EXPECT_TRUE(Px(0, 0) == 2 && Px(8, 0) == 3 && Px(16, 0) == 3) << "forward copy";
  FillPx(0);
  SetPx(0, 0, 5);
  opeu_screen_copy(0, 0, 1, 1, 3, 2);
  EXPECT_EQ(Px(24, 2), 5) << "copy to another line";
  // Rows overlapping downwards: the block moves intact (bottom row first).
  FillPx(0);
  SetPx(0, 0, 1);
  SetPx(0, 1, 2);
  SetPx(0, 2, 3);
  opeu_screen_copy(0, 0, 1, 3, 0, 1);
  EXPECT_TRUE(Px(0, 1) == 1 && Px(0, 2) == 2 && Px(0, 3) == 3) << "overlapping rows downwards";
  FillPx(0);
  SetPx(0, 1, 1);
  SetPx(0, 2, 2);
  SetPx(0, 3, 3);
  opeu_screen_copy(0, 1, 1, 3, 0, 0);
  EXPECT_TRUE(Px(0, 0) == 1 && Px(0, 1) == 2 && Px(0, 2) == 3) << "overlapping rows upwards";
}

// Plain: an 8x2 picture at (8, 3), 2 units per row. Row 0: a literal of 2 units with value 421h (plane 2 = 4, plane 1
// = 2, plane 0 = 1, the bit patterns of 4 pixels); row 1: copy 2 units from 1 row up (1 1 00 0001 = C1h).
constexpr std::array<std::uint8_t, 7> PLAIN_PICTURE{ 8, 0, 2, 0, 0x14, 0x21, 0xC1 };

TEST_F(Vgalib, DecodesPlainPictures) {
  Stream stream{ PLAIN_PICTURE };
  FillPx(0);
  EXPECT_EQ(opeu_picture_decode(8, 3, stream.Reader()), 0);
  // Plane 0 nibble 1 = 0001: pixel 3 only; plane 1 nibble 2 = 0010: pixel 2; plane 2 nibble 4 = 0100: pixel 1.
  auto const expected{ [](int const x) { return std::array{ 0, 4, 2, 1 }[static_cast<std::size_t>(x % 4)]; } };
  ExpectRow(3, { 8, 16 }, [&](int const x) { return expected(x - 8); });
  ExpectRow(4, { 8, 16 }, [&](int const x) { return expected(x - 8); });
  EXPECT_EQ(stream.Consumed(), PLAIN_PICTURE.size()) << "plain stream consumed";
}

TEST_F(Vgalib, DecodesFromACapturingReader) {
  FillPx(0);
  std::size_t read{};
  auto const  reader{ [&read] { return PLAIN_PICTURE[read++]; } }; // an injected reader with state of its own
  EXPECT_EQ(opeu_picture_decode(8, 3, reader), 0);
  EXPECT_TRUE(read == PLAIN_PICTURE.size() && Px(9, 3) == 4 && Px(11, 4) == 1);
}

TEST_F(Vgalib, DecodesMaskedPictures) {
  constexpr std::array<std::uint8_t, 21> MASKED{
    4,    0, 1, 0, 0x34, 0x12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // run 0 = 1234h
    0x40,                                                                // run 0, 1 unit
  };
  constexpr std::array<std::uint8_t, 24> FULL_LITERAL{
    8,    0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x38, 0xAB, 0xCD, // 0111 000 -> 1 unit, value ABCDh
    0x80,                                                                            // copy 1 unit from 1 back
  };
  Stream masked{ MASKED };
  FillPx(0);
  EXPECT_EQ(opeu_picture_decode_masked(4, 0, masked.Reader()), 0);
  // Value 1234h: plane 0 = 4 (0100) pixel 1, plane 1 = 3 (0011) pixels 2, 3, plane 2 = 2 (0010) pixel 2, plane 3 = 1
  // (0001) pixel 3.
  ExpectRow(0, { 4, 8 }, [](int const x) { return std::array{ 0, 1, 6, 10 }[static_cast<std::size_t>(x - 4)]; });
  Stream full{ FULL_LITERAL };
  FillPx(0);
  std::ignore = opeu_picture_decode_masked(0, 0, full.Reader());
  // ABCDh: plane 0 = D (1101), plane 1 = C (1100), plane 2 = B (1011), plane 3 = A (1010); then the copy of 1 unit
  // back (odd parity, the rotate path).
  ExpectRow(0, { 0, 8 }, [](int const x) { return std::array{ 15, 3, 12, 5 }[static_cast<std::size_t>(x % 4)]; });
}

TEST_F(Vgalib, DecodesRepeatsAndRejectsWidePictures) {
  constexpr std::array<std::uint8_t, 7> REPEAT{ 16, 0, 1, 0, 0x00, 0x0F, 0x82 }; // 1 literal + copy 3
  constexpr std::array<std::uint8_t, 2> TOO_WIDE{ 0x81, 0x02 };
  Stream                                repeat{ REPEAT };
  FillPx(0);
  std::ignore = opeu_picture_decode(0, 0, repeat.Reader());
  ExpectRow(0, { 0, 16 }, [](int) { return 1; });
  Stream too_wide{ TOO_WIDE };
  EXPECT_EQ(opeu_picture_decode(0, 0, too_wide.Reader()), -1) << "width 641 rejected";
}

TEST_F(Vgalib, LoadsThePaletteAndRendersTheFrame) {
  constexpr std::array<std::uint8_t, 48>   RGB{ 1, 2, 3, 15, 0, 15 };
  std::vector<std::uint8_t>                out(640 * 480);
  std::array<std::uint8_t, 768>            palette{};
  std::span<std::uint8_t, 640 * 480> const frame{ out.data(), out.size() };
  koei_vga_identity_palette();
  EXPECT_EQ(opeu_set_palette(1, 2, RGB.data()), 0);
  // Entry 1: R = b1 << 2 = 8, G = b2 << 2 = 12, B = b0 << 2 = 4.
  auto const dac{ Screen().Palette() };
  EXPECT_TRUE(dac[3] == 8 && dac[4] == 12 && dac[5] == 4) << "dac entry 1";
  EXPECT_TRUE(dac[6] == 0 && dac[7] == 60 && dac[8] == 60) << "dac entry 2";
  opeu_set_palette(15, 2, RGB.data()); // runs past 15
  EXPECT_TRUE(dac[16 * 3] == 0 && dac[16 * 3 + 1] == 60) << "dac entry 16 written";
  opeu_set_palette(4, 20, RGB.data()); // count -> 16 - 4 = 12 (reads 12 entries)
  SetPx(0, 0, 1);
  SetPx(639, 399, 2);
  EXPECT_TRUE(VgalibRender(frame, palette)) << "render dirty";
  EXPECT_TRUE(out[40 * 640] == 1 && out[439 * 640 + 639] == 2 && out[0] == 0) << "render pixels";
  EXPECT_TRUE(palette[3] == ((8 << 2) | (8 >> 4)) && palette[4] == ((12 << 2) | (12 >> 4))) << "palette scale";
  EXPECT_FALSE(VgalibRender(frame, palette)) << "render clean";
  Screen().MapColour(1, 0x3F);
  std::ignore = VgalibRender(frame, palette);
  EXPECT_EQ(out[40 * 640], 0x3F) << "colour map applied";
  opeu_text_mode();
  std::ignore = VgalibRender(frame, palette);
  EXPECT_EQ(out[40 * 640], 0) << "text mode renders blank";
}

}
}
