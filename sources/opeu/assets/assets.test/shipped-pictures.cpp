// The picture-stream files: every picture decodes at the size the game draws it, inside its file.
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/picture-stream.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::assets {

namespace {

using test::Shipped;

struct Size {
  std::size_t width;
  std::size_t height;
  auto        operator==(Size const&) const -> bool = default;
};

// The assets of one file in catalogue order.
auto AssetsOf(std::string_view const file) -> std::vector<Asset> {
  auto const  shipped{ test::ShippedCatalogue{} };
  auto const& catalogue{ shipped.Catalogued() };
  return catalogue.Assets() | std::views::filter([file](Asset const& asset) { return asset.file == file; }) | std::ranges::to<std::vector>();
}

// Each picture of a file decoded, with its offset and stream length.
auto PicturesOf(std::string_view const file) -> std::vector<std::pair<Asset, Picture>> {
  auto const contents{ Shipped(file) };
  auto const bytes{ Bytes{ contents } };
  return AssetsOf(file) | std::views::transform([bytes](Asset const& asset) {
           auto const colours{ asset.layout == Layout::STREAM_8 ? Colours::EIGHT : Colours::SIXTEEN };
           auto const picture{ DecodePicture(bytes.subspan(asset.at), colours) };
           EXPECT_TRUE(picture) << asset.id;
           return std::pair{ asset, picture.value_or(Picture{}) };
         })
         | std::ranges::to<std::vector>();
}

auto SizeOf(Picture const& picture) -> Size {
  return { picture.image.Width(), picture.image.Height() };
}

auto ExpectAllOfSize(std::span<std::pair<Asset, Picture> const> const pictures, Size const size) -> void {
  for (auto const& [asset, picture] : pictures) EXPECT_EQ(SizeOf(picture), size) << asset.id;
}

auto ExpectSizes(std::span<std::pair<Asset, Picture> const> const pictures, std::span<Size const> const sizes) -> void {
  for (auto const& [entry, size] : std::views::zip(pictures, sizes)) EXPECT_EQ(SizeOf(entry.second), size) << entry.first.id;
}

auto ExpectLengths(std::span<std::pair<Asset, Picture> const> const pictures, std::span<std::size_t const> const lengths) -> void {
  for (auto const& [entry, length] : std::views::zip(pictures, lengths)) EXPECT_EQ(entry.second.length, length) << entry.first.id;
}

// Each picture ends where the next one starts.
auto ExpectBackToBack(std::span<std::pair<Asset, Picture> const> const pictures) -> void {
  for (auto const& [current, next] : std::views::zip(pictures, pictures | std::views::drop(1))) {
    EXPECT_EQ(current.first.at + current.second.length, next.first.at) << current.first.id;
  }
}

// Each picture ends inside its file.
auto ExpectInside(std::span<std::pair<Asset, Picture> const> const pictures, std::size_t const file_size) -> void {
  for (auto const& [asset, picture] : pictures) EXPECT_LE(asset.at + picture.length, file_size) << asset.id;
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
  ExpectAllOfSize(pictures, Size{ 256, 168 });
  ExpectBlocksDoNotOverlap(pictures);
  EXPECT_EQ(Shipped("EVENT.DAT").size(), 510U * 512U);
}

TEST(ShippedSelection, HoldsTheTitleAndSixScenarioPictures) {
  auto const pictures{ PicturesOf("SCESEL.DAT") };
  ASSERT_EQ(pictures.size(), 7U);
  EXPECT_EQ(SizeOf(pictures[0].second), (Size{ 640, 400 }));
  EXPECT_EQ(pictures[0].second.length, 14'844U);
  auto const scenarios{ std::span{ pictures }.subspan(1) };
  ExpectAllOfSize(scenarios, Size{ 256, 128 });
  constexpr auto LENGTHS{
    std::array{ 7'605uz, 3'295uz, 6'729uz, 5'435uz, 7'454uz, 5'765uz }
  };
  ExpectLengths(scenarios, LENGTHS);
  ExpectBlocksDoNotOverlap(pictures);
}

TEST(ShippedOutcome, HoldsFourOutcomesAndSixMedals) {
  auto const pictures{ PicturesOf("SCEEND.DAT") };
  ASSERT_EQ(pictures.size(), 10U);
  auto const outcomes{ std::ranges::count_if(pictures, [](auto const& entry) { return SizeOf(entry.second) == Size{ 480, 240 }; }) };
  auto const medals{ std::ranges::count_if(pictures, [](auto const& entry) { return SizeOf(entry.second) == Size{ 96, 144 }; }) };
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
  auto const bars{ std::span{ pictures }.subspan(3) };
  ExpectAllOfSize(bars, Size{ 352, 32 });
  ExpectInside(bars, Shipped("MGAMEN.DAT").size());
}

TEST(ShippedEpilogue, HoldsSevenPicturesBackToBackAndABlankToken) {
  auto const pictures{ PicturesOf("ENDGRP.NPK") };
  ASSERT_EQ(pictures.size(), 7U);
  constexpr auto SIZES{ std::array<Size, 7>{ { { 640, 160 }, { 640, 224 }, { 320, 224 }, { 320, 224 }, { 640, 184 }, { 640, 368 }, { 384, 160 } } } };
  ExpectSizes(pictures, SIZES);
  ExpectBackToBack(pictures);
  EXPECT_EQ(pictures.back().first.at + pictures.back().second.length, 0x145F5U);
  auto const contents{ Shipped("ENDGRP.NPK") };
  auto const bytes{ Bytes{ contents } };
  ASSERT_EQ(bytes.size(), EPILOGUE_TOKEN_AT + EPILOGUE_TOKEN_BYTES);
  EXPECT_TRUE(std::ranges::all_of(bytes.subspan(EPILOGUE_TOKEN_AT), [](std::byte const b) { return b == std::byte{ ' ' }; }));
}

TEST(ShippedOpening, HoldsThirtyPicturesThatFillTheFile) {
  auto const pictures{ PicturesOf("OPENGRP.NPK") };
  ASSERT_EQ(pictures.size(), 30U);
  ExpectBackToBack(pictures);
  EXPECT_EQ(pictures.back().first.at + pictures.back().second.length, Shipped("OPENGRP.NPK").size());
  EXPECT_EQ(SizeOf(pictures[28].second), (Size{ 520, 240 })); // the OPERATION EUROPE logo
}

}
