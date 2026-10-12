// The layouts every decoder is built from, on small hand-made inputs.
#include "opeu/assets/palette.hpp"
#include "opeu/assets/picture-stream.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/assets/runs.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>

namespace opeu::assets {

namespace {

template <class... Values>
constexpr auto Of(Values... values) -> std::array<std::byte, sizeof...(Values)> {
  return { std::byte(values)... };
}

}

TEST(Planar, InterleavedPlanesMakeTheIndexBits) {
  auto const bytes{ Of(0x80, 0x80, 0x00, 0x80, 0x01, 0x00, 0x01, 0x00) }; // pixel 0: planes 0,1,3; pixel 15: planes 0,2
  auto const image{ DecodeInterleaved(bytes, 2, 1, 4) };
  ASSERT_TRUE(image);
  EXPECT_EQ(image->Width(), 16);
  EXPECT_EQ(image->Height(), 1);
  EXPECT_EQ(image->At(0, 0), 0b1011);
  EXPECT_EQ(image->At(15, 0), 0b0101);
}

TEST(Planar, RefuseTooFewBytes) {
  auto const bytes{ Of(0, 0, 0) };
  EXPECT_EQ(DecodeInterleaved(bytes, 1, 1, 4).error(), DecodeError::TRUNCATED);
  EXPECT_EQ(DecodeCell(bytes).error(), DecodeError::TRUNCATED);
}

TEST(Runs, ZeroAndFfRepeatOtherBytesStand) {
  auto const bytes{ Of(0x00, 0x02, 0x42, 0xFF, 0x00, 0x17) };
  auto const expanded{ ExpandRuns(bytes, 5) };
  ASSERT_TRUE(expanded);
  EXPECT_EQ(expanded->consumed, 5U);
  EXPECT_EQ(expanded->bytes,
            (std::vector{ std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0x42 }, std::byte{ 0xFF } }));
  EXPECT_EQ(ExpandRuns(bytes, 9).error(), DecodeError::TRUNCATED);
}

TEST(PictureStream, EightColourLiteralAndCopies) {
  // 8x2: a literal unit of colour 1 (plane 0 set), a copy of it one unit back, then the row above copied down.
  auto const bytes{ Of(8, 0, 2, 0, 0x00, 0x0F, 0x80, 0xC1) };
  auto const picture{ DecodePicture(bytes, Colours::EIGHT) };
  ASSERT_TRUE(picture);
  EXPECT_EQ(picture->length, bytes.size());
  for (auto y{ 0uz }; y < 2; ++y) {
    for (auto x{ 0uz }; x < 8; ++x) EXPECT_EQ(picture->image.At(x, y), 1) << x << ',' << y;
  }
}

TEST(PictureStream, RefuseAnOffScreenSize) {
  auto const bytes{ Of(0x81, 0x02, 1, 0) };
  EXPECT_EQ(DecodePicture(bytes, Colours::EIGHT).error(), DecodeError::BAD_HEADER);
  EXPECT_EQ(DecodePicture(Of(8, 0, 1, 0, 0x00), Colours::EIGHT).error(), DecodeError::TRUNCATED);
}

TEST(Palette, LevelsBecomeTheScreensColours) {
  EXPECT_EQ(Intensity(0), 0);
  EXPECT_EQ(Intensity(15), 243);
  auto const day{ FindPalette("main/day") };
  ASSERT_TRUE(day);
  auto const colours{ Colours(day->levels) };
  EXPECT_EQ(colours[7], (Colour{ 243, 243, 243 }));
  EXPECT_EQ(colours[1].b, Intensity(9)); // index bit 0 is blue
  EXPECT_EQ(ClassicPalettes().size(), 121U);
  EXPECT_EQ(FindPalette("open/112")->use, "the opening's alternative fade target");
  EXPECT_FALSE(FindPalette("none"));
}

}
