// The picture-stream files: every picture decodes at the size the game draws it, inside its file.
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/picture-stream.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::assets {

namespace {

using test::Embedded;
using test::Shipped;

struct Size {
  std::size_t width;
  std::size_t height;
  auto operator==(Size const&) const -> bool = default;
};

// The assets of one file in catalogue order.
auto AssetsOf(std::string_view const file) -> std::vector<Asset> {
  auto const catalogue{ Catalogue{ Embedded() } };
  return catalogue.Assets() | std::views::filter([file](Asset const& asset) { return asset.file == file; })
         | std::ranges::to<std::vector>();
}

// Each picture of a file decoded, with its offset and stream length.
auto PicturesOf(std::string_view const file) -> std::vector<std::pair<Asset, Picture>> {
  auto const bytes{ Shipped(file) };
  return AssetsOf(file) | std::views::transform([bytes](Asset const& asset) {
           auto const colours{ asset.layout == Layout::STREAM_8 ? Colours::EIGHT : Colours::SIXTEEN };
           auto picture{ DecodePicture(bytes.subspan(asset.at), colours) };
           EXPECT_TRUE(picture) << asset.id;
           return std::pair{ asset, picture.value_or(Picture{}) };
         })
         | std::ranges::to<std::vector>();
}

auto SizeOf(Picture const& picture) -> Size {
  return { picture.image.Width(), picture.image.Height() };
}

// Pictures that start on 512-byte blocks each end before the next one starts.
auto ExpectBlocksDoNotOverlap(std::vector<std::pair<Asset, Picture>> pictures) -> void {
  std::ranges::sort(pictures, {}, [](auto const& entry) { return entry.first.at; });
  for (auto const& [current, next] : std::views::zip(pictures, pictures | std::views::drop(1))) {
    EXPECT_LE(current.first.at + current.second.length, next.first.at) << current.first.id;
  }
}

}

TEST(ShippedEvent, HoldsThirtyTwoEventPictures) {
  auto const pictures{ PicturesOf("EVENT.DAT") };
  ASSERT_EQ(pictures.size(), 32U);
  for (auto const& [asset, picture] : pictures) EXPECT_EQ(SizeOf(picture), (Size{ 256, 168 })) << asset.id;
  ExpectBlocksDoNotOverlap(pictures);
  EXPECT_EQ(Shipped("EVENT.DAT").size(), 510U * 512U);
}

TEST(ShippedSelection, HoldsTheTitleAndSixScenarioPictures) {
  auto const pictures{ PicturesOf("SCESEL.DAT") };
  ASSERT_EQ(pictures.size(), 7U);
  EXPECT_EQ(SizeOf(pictures[0].second), (Size{ 640, 400 }));
  EXPECT_EQ(pictures[0].second.length, 14'844U);
  for (auto const& [asset, picture] : pictures | std::views::drop(1)) {
    EXPECT_EQ(SizeOf(picture), (Size{ 256, 128 })) << asset.id;
  }
  constexpr auto LENGTHS{ std::array{ 7'605uz, 3'295uz, 6'729uz, 5'435uz, 7'454uz, 5'765uz } };
  for (auto const& [entry, length] : std::views::zip(pictures | std::views::drop(1), LENGTHS)) {
    EXPECT_EQ(entry.second.length, length) << entry.first.id;
  }
  ExpectBlocksDoNotOverlap(pictures);
}

TEST(ShippedOutcome, HoldsFourOutcomesAndSixMedals) {
  auto const pictures{ PicturesOf("SCEEND.DAT") };
  ASSERT_EQ(pictures.size(), 10U);
  auto const outcomes{ std::ranges::count_if(
      pictures, [](auto const& entry) { return SizeOf(entry.second) == Size{ 480, 240 }; }) };
  auto const medals{ std::ranges::count_if(pictures,
                                           [](auto const& entry) { return SizeOf(entry.second) == Size{ 96, 144 }; }) };
  EXPECT_EQ(outcomes, 4);
  EXPECT_EQ(medals, 6);
  ExpectBlocksDoNotOverlap(pictures);
}

TEST(ShippedFrame, HoldsTheMainScreensPieces) {
  auto const pictures{ PicturesOf("MGAMEN.DAT") };
  ASSERT_EQ(pictures.size(), 9U);
  EXPECT_EQ(SizeOf(pictures[0].second), (Size{ 640, 24 }));
  EXPECT_EQ(SizeOf(pictures[1].second), (Size{ 352, 344 }));
  EXPECT_EQ(SizeOf(pictures[2].second), (Size{ 288, 376 }));
  for (auto const& [asset, picture] : pictures | std::views::drop(3)) {
    EXPECT_EQ(SizeOf(picture), (Size{ 352, 32 })) << asset.id;
    EXPECT_LE(asset.at + picture.length, Shipped("MGAMEN.DAT").size()) << asset.id;
  }
}

TEST(ShippedEpilogue, HoldsSevenPicturesBackToBackAndABlankToken) {
  auto const pictures{ PicturesOf("ENDGRP.NPK") };
  ASSERT_EQ(pictures.size(), 7U);
  constexpr auto SIZES{ std::array<Size, 7>{
      { { 640, 160 }, { 640, 224 }, { 320, 224 }, { 320, 224 }, { 640, 184 }, { 640, 368 }, { 384, 160 } } } };
  for (auto const& [entry, size] : std::views::zip(pictures, SIZES))
    EXPECT_EQ(SizeOf(entry.second), size) << entry.first.id;
  for (auto const& [current, next] : std::views::zip(pictures, pictures | std::views::drop(1))) {
    EXPECT_EQ(current.first.at + current.second.length, next.first.at) << current.first.id;
  }
  EXPECT_EQ(pictures.back().first.at + pictures.back().second.length, 0x145F5U);
  auto const bytes{ Shipped("ENDGRP.NPK") };
  ASSERT_EQ(bytes.size(), EPILOGUE_TOKEN_AT + EPILOGUE_TOKEN_BYTES);
  EXPECT_TRUE(
      std::ranges::all_of(bytes.subspan(EPILOGUE_TOKEN_AT), [](std::byte const b) { return b == std::byte{ ' ' }; }));
}

TEST(ShippedOpening, HoldsThirtyPicturesThatFillTheFile) {
  auto const pictures{ PicturesOf("OPENGRP.NPK") };
  ASSERT_EQ(pictures.size(), 30U);
  for (auto const& [current, next] : std::views::zip(pictures, pictures | std::views::drop(1))) {
    EXPECT_EQ(current.first.at + current.second.length, next.first.at) << current.first.id;
  }
  EXPECT_EQ(pictures.back().first.at + pictures.back().second.length, Shipped("OPENGRP.NPK").size());
  EXPECT_EQ(SizeOf(pictures[28].second), (Size{ 520, 240 }));  // the OPERATION EUROPE logo
}

}
