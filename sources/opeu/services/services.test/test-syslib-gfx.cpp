// Checks of the battle-cell and sprite routines against values worked out by hand, in the bit-per-colour form of the
// game's buffers.
#include "syslib-gfx.hpp"

#include "canvas.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>

namespace opeu::services {
namespace {

using CellBuffer = std::array<std::uint8_t, SYSLIB_CELL_BYTES>;

// Bit `plane` of the 8 pixels of screen column `column` (80 columns per screen line).
auto Plane(int const plane, int const column) -> int {
  return Screen().PlaneBits(Offset{ column } * 8, plane);
}

auto SetPlane(int const plane, int const column, int const bits) -> void {
  Screen().SetPlaneBits(Offset{ column } * 8, plane, static_cast<std::uint8_t>(bits));
}

// Sets bits `bits` in every colour bit of the screen columns [from, from + count).
auto SetAllPlanes(int const from, int const count, int const bits) -> void {
  std::ranges::for_each(std::views::iota(0, 4), [&](int const plane) { std::ranges::for_each(std::views::iota(from, from + count), [&](int const column) { SetPlane(plane, column, bits); }); });
}

class SyslibGfx : public testing::Test {
protected:
  auto SetUp() -> void override {
    Screen().Clear();
  }
};

TEST_F(SyslibGfx, RestoresACellHalfAColumnRight) { // 354F
  constexpr auto COLUMN{ (40 * 80) + 2 };
  CellBuffer     cell{ 0x12, 0x34, 0x56, 0x78, 0x9A };
  SetAllPlanes(COLUMN, 6, 0xAA);
  opeu_vram_restore_odd(2, 0, cell.data());
  EXPECT_EQ(Plane(0, COLUMN), 0xA1);
  EXPECT_EQ(Plane(0, COLUMN + 1), 0x23);
  EXPECT_EQ(Plane(0, COLUMN + 2), 0x45);
  EXPECT_EQ(Plane(0, COLUMN + 3), 0x67);
  EXPECT_EQ(Plane(0, COLUMN + 4), 0x89);
  EXPECT_EQ(Plane(0, COLUMN + 5), 0xAA);
  EXPECT_TRUE(Plane(1, COLUMN) == 0xA0 && Plane(1, COLUMN + 5) == 0x0A);
  EXPECT_EQ(Plane(1, COLUMN + 6), 0x00);
}

TEST_F(SyslibGfx, SavesAndRestoresCells) { // 3882, 37E4, 34B8
  constexpr std::array<std::uint8_t, 6> SAVED_ROW{ 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC };
  CellBuffer                            cell{};
  CellBuffer                            again{};
  std::ranges::for_each(std::views::iota(0, 6), [&](int const i) { SetPlane(0, 3200 + i, SAVED_ROW[static_cast<std::size_t>(i)]); });
  opeu_vram_save_odd(0, 0, cell.data());
  EXPECT_TRUE(cell[0] == 0x23 && cell[1] == 0x45 && cell[2] == 0x67 && cell[3] == 0x89 && cell[4] == 0xAB);
  EXPECT_EQ(cell[5], 0);
  std::ranges::transform(std::views::iota(0uz, cell.size()), cell.begin(), [](std::size_t const i) { return static_cast<std::uint8_t>((i * 7) + 3); });
  opeu_vram_restore(10, 100, cell.data());
  EXPECT_EQ(Plane(2, (100 + 40) * 80 + 10 + 80 + 3), cell[20 + 10 + 3]);
  opeu_vram_save(10, 100, again.data());
  EXPECT_EQ(cell, again);
  opeu_vram_restore_odd(20, 50, cell.data());
  opeu_vram_save_odd(20, 50, again.data());
  EXPECT_EQ(cell, again);
}

TEST_F(SyslibGfx, SpillsGlyphPairsAcrossColourBitBlocks) { // 3A4D: the word operations cross the blocks
  constexpr std::array<std::uint8_t, 6> GLYPH{ 0x81, 0, 0, 0, 0, 0 };
  std::array<std::uint8_t, 540>         cell{};
  std::array<std::uint8_t, 540>         expected{};
  cell.fill(0xFF);
  expected.fill(0xFF);
  expected[4]  = 0xBF;
  expected[5]  = 0x7F;
  expected[14] = 0xBF;
  expected[15] = 0x7F;
  expected[19] = 0xBF;
  expected[20] = 0x7F;
  opeu_vram_glyph_color(cell.data(), GLYPH.data(), 33, 0, 0);
  EXPECT_EQ(cell, expected);
  cell.fill(0);
  opeu_vram_glyph(cell.data(), GLYPH.data(), 0, 1); // colour bits 0-2 get 81h, bit 3 cleared
  EXPECT_TRUE(cell[20] == 0x81 && cell[25] == 0x81 && cell[30] == 0x81 && cell[35] == 0 && cell[21] == 0);
}

TEST_F(SyslibGfx, ComposesSprites) { // 3E0A
  constexpr std::array<std::uint8_t, 2> MASK{ 0xF0, 0x0F };
  constexpr std::array<std::uint8_t, 6> IMAGE{ 0x01, 0x02, 0x04, 0x10, 0x20, 0x40 };
  std::array<std::uint8_t, 16>          buffer{};
  SetPlane(0, 3200, 0xFF);
  SetPlane(0, 3201, 0xFF);
  SetPlane(1, 3200, 0xFF);
  SetPlane(1, 3201, 0x00);
  SetPlane(2, 3200, 0x00);
  SetPlane(2, 3201, 0xFF);
  SetPlane(3, 3200, 0x0F);
  SetPlane(3, 3201, 0xF0);
  opeu_sprite_compose(0, 0, 2, 1, MASK.data(), IMAGE.data(), buffer.data(), 0);
  EXPECT_TRUE(Plane(0, 3200) == 0xF1 && Plane(0, 3201) == 0x1F);
  EXPECT_TRUE(Plane(1, 3200) == 0xF2 && Plane(1, 3201) == 0x20);
  EXPECT_TRUE(Plane(2, 3200) == 0x04 && Plane(2, 3201) == 0x4F);
  EXPECT_TRUE(Plane(3, 3200) == 0x00 && Plane(3, 3201) == 0x00);
  EXPECT_TRUE(buffer[0] == 0xF1 && buffer[3] == 0x20 && buffer[6] == 0x00);
}

TEST_F(SyslibGfx, ComposesSpritesShiftedByPixels) { // 3F26 with a 4-pixel shift
  constexpr std::array<std::uint8_t, 1> MASK{ 0x00 };
  constexpr std::array<std::uint8_t, 4> IMAGE{ 0x0F, 0xF0, 0x3C, 0xC3 };
  std::array<std::uint8_t, 8>           buffer{};
  SetAllPlanes(3200, 2, 0xFF);
  koei_3f26(4, 0, 8, 1, MASK.data(), IMAGE.data(), buffer.data(), 1);
  EXPECT_TRUE(Plane(0, 3200) == 0xF0 && Plane(0, 3201) == 0xFF);
  EXPECT_TRUE(Plane(1, 3200) == 0xFF && Plane(1, 3201) == 0x0F);
  EXPECT_TRUE(Plane(2, 3200) == 0xF3 && Plane(2, 3201) == 0xCF);
  EXPECT_TRUE(Plane(3, 3200) == 0xFC && Plane(3, 3201) == 0x3F);
  // Flags bit 0 clear: the original never returns; the port leaves the screen alone.
  SetAllPlanes(3200, 2, 0xFF);
  koei_3f26(4, 0, 8, 1, MASK.data(), IMAGE.data(), buffer.data(), 0);
  EXPECT_TRUE(Plane(0, 3200) == 0xFF && Plane(3, 3201) == 0xFF);
}

TEST_F(SyslibGfx, CopiesBlocksAndFillsColourBitThree) { // 3BC5 overlap, 3B77
  SetPlane(0, 3200, 1);
  SetPlane(0, 3201, 2);
  SetPlane(0, 3202, 3);
  SetPlane(0, 3203, 4);
  koei_3bc5(0, 0, 1, 0);
  EXPECT_TRUE(Plane(0, 3201) == 1 && Plane(0, 3202) == 2 && Plane(0, 3203) == 2);
  Screen().Clear();
  Koei3b77(5, 2, 3, 2, 0x1A5);
  EXPECT_TRUE(Plane(3, 42 * 80 + 5) == 0xA5 && Plane(3, 43 * 80 + 7) == 0xA5);
  EXPECT_TRUE(Plane(3, 43 * 80 + 8) == 0 && Plane(3, 44 * 80 + 5) == 0);
  EXPECT_EQ(Plane(0, 42 * 80 + 5), 0);
}

TEST_F(SyslibGfx, MasksAndDrawsCells) { // 3B1E, 3A04
  CellBuffer                    cell{};
  std::array<std::uint8_t, 130> mask{};
  CellBuffer                    source{};
  cell.fill(0xFF);
  std::ranges::transform(std::views::iota(0uz, mask.size()), mask.begin(), [](std::size_t const i) { return static_cast<std::uint8_t>(i | 0x80); });
  opeu_vram_load(cell.data(), mask.data());
  EXPECT_TRUE(cell[0] == 0x80 && cell[5] == 0x80 && cell[15 + 4] == 0x84 && cell[20] == 0x85 && cell[519] == (129 | 0x80));
  source.fill(0x01);
  opeu_vram_draw(cell.data(), source.data());
  EXPECT_TRUE(cell[0] == 0x81 && cell[519] == 0x81);
}

}
}
