// The raw bitmap files: tiles, battle sprites, portraits, silhouettes, dialogs, masks, logos and glyph fonts.
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/runs.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

using test::Embedded;
using test::Shipped;

// Every asset of a file decoded; each must decode.
auto ImagesOf(std::string_view const file) -> std::vector<Image> {
  auto const catalogue{ Catalogue{ Embedded() } };
  return catalogue.Assets() | std::views::filter([file](Asset const& asset) { return asset.file == file; })
         | std::views::transform([&catalogue](Asset const& asset) {
             auto image{ catalogue.Decode(asset) };
             EXPECT_TRUE(image) << asset.id;
             return image.value_or(Image{});
           })
         | std::ranges::to<std::vector>();
}

auto AllSized(std::vector<Image> const& images, std::size_t const width, std::size_t const height) -> bool {
  return std::ranges::all_of(images,
                             [&](Image const& image) { return image.Width() == width && image.Height() == height; });
}

auto Highest(Image const& image) -> std::uint8_t {
  return std::ranges::max(image.Pixels());
}

}

TEST(ShippedOverviewTiles, HoldNinetySixTilesPerMap) {
  auto const tiles{ ImagesOf("BIGMAP.DAT") };
  EXPECT_EQ(tiles.size(), 6U * 96U);
  EXPECT_TRUE(AllSized(tiles, 8, 8));
  EXPECT_EQ(Shipped("BIGMAP.DAT").size(), tiles.size() * 32);
}

TEST(ShippedViewTiles, HoldNinetySixTilesPerMap) {
  auto const tiles{ ImagesOf("MIDMAP.DAT") };
  EXPECT_EQ(tiles.size(), 6U * 96U);
  EXPECT_TRUE(AllSized(tiles, 24, 24));
  EXPECT_EQ(Shipped("MIDMAP.DAT").size(), tiles.size() * 288);
}

TEST(ShippedBattleTerrain, HoldNinetyFiveMaskedCellsPerSeason) {
  auto const summer{ ImagesOf("HEXFONT.DAT") };
  auto const snow{ ImagesOf("HEXFONTS.DAT") };
  ASSERT_EQ(summer.size(), 95U);
  ASSERT_EQ(snow.size(), 95U);
  EXPECT_TRUE(AllSized(summer, 40, 26));
  EXPECT_TRUE(std::ranges::all_of(summer, &Image::HasMask));
  auto const differing{ std::ranges::count_if(std::views::zip(summer, snow), [](auto const& pair) {
    auto const& [a, b]{ pair };
    return a != b;
  }) };
  EXPECT_EQ(differing, 77);
}

TEST(ShippedBattleUnits, HoldIconsBlastsAndTwoBorders) {
  auto const sprites{ ImagesOf("HEXUNIT.DAT") };
  ASSERT_EQ(sprites.size(), 54U);
  EXPECT_TRUE(AllSized(sprites | std::views::take(52) | std::ranges::to<std::vector>(), 40, 26));
  EXPECT_TRUE(AllSized(sprites | std::views::drop(52) | std::ranges::to<std::vector>(), 24, 20));
  EXPECT_EQ(Shipped("HEXUNIT.DAT").size(), 52U * 650U + 2U * 300U);
  auto const swapped{ SideZeroColours(sprites.front()) };
  EXPECT_NE(swapped, sprites.front());
  EXPECT_EQ(SideZeroColours(swapped), sprites.front());
}

TEST(ShippedPortraits, HoldOneHundredTwentyEightColourFaces) {
  auto const faces{ ImagesOf("FACE.DAT") };
  ASSERT_EQ(faces.size(), 120U);
  EXPECT_TRUE(AllSized(faces, 64, 80));
  EXPECT_TRUE(std::ranges::all_of(faces, [](Image const& face) { return Highest(face) < 8; }));
  EXPECT_EQ(Shipped("FACE.DAT").size(), 120U * 1920U);
}

TEST(ShippedSilhouettes, HoldOneHundredFiftyNineRunPackedRecords) {
  auto const catalogue{ Catalogue{ Embedded() } };
  auto const records{ catalogue.Assets() | std::views::filter([](Asset const& a) { return a.file == "ARMS.SIL"; })
                      | std::ranges::to<std::vector>() };
  ASSERT_EQ(records.size(), 159U);
  auto const bytes{ Shipped("ARMS.SIL") };
  auto const ends{ std::views::zip(records, records | std::views::drop(1)) };
  EXPECT_TRUE(std::ranges::all_of(ends, [bytes](auto const& pair) {
    auto const& [record, next]{ pair };
    auto const expanded{ ExpandRuns(bytes.subspan(record.at), 0x780) };
    return expanded && record.at + expanded->consumed <= next.at;
  }));
  EXPECT_EQ(records.back().at + ExpandRuns(bytes.subspan(records.back().at), 0x780)->consumed, 121'137U);
  EXPECT_TRUE(AllSized(ImagesOf("ARMS.SIL"), 192, 80));
}

TEST(ShippedDialogs, HoldTheNumberDialogFourButtonsAndThePanel) {
  auto const bitmaps{ ImagesOf("MSGRP.DAT") };
  ASSERT_EQ(bitmaps.size(), 6U);
  EXPECT_EQ(bitmaps[0].Width(), 144U);
  EXPECT_EQ(bitmaps[0].Height(), 96U);
  EXPECT_TRUE(AllSized(bitmaps | std::views::drop(1) | std::views::take(4) | std::ranges::to<std::vector>(), 32, 16));
  EXPECT_EQ(bitmaps[5].Width(), 256U);
  EXPECT_EQ(bitmaps[5].Height(), 120U);
  EXPECT_EQ(Shipped("MSGRP.DAT").size(), 0x5B00U);
}

TEST(ShippedOpeningMasks, HoldSixBlastMasksAndSixShells) {
  auto const masks{ ImagesOf("MOPENGRP.DAT") };
  ASSERT_EQ(masks.size(), 12U);
  EXPECT_TRUE(AllSized(masks | std::views::take(6) | std::ranges::to<std::vector>(), 208, 176));
  EXPECT_EQ(masks[6].Width(), 64U);
  EXPECT_EQ(masks[6].Height(), 40U);
  EXPECT_EQ(Shipped("MOPENGRP.DAT").size(), 6U * 4576U + 512U);
}

TEST(ShippedEndingMasks, HoldEightAircraftFrames) {
  auto const masks{ ImagesOf("MENDGRP.DAT") };
  ASSERT_EQ(masks.size(), 8U);
  EXPECT_TRUE(AllSized(masks, 256, 64));
  EXPECT_EQ(Shipped("MENDGRP.DAT").size(), 8U * 2048U);
}

TEST(ShippedLogo, HoldsFourOneBitCaptionsAndAnEndOfFileMark) {
  auto const logos{ ImagesOf("KOEILOGO.FNT") };
  ASSERT_EQ(logos.size(), 4U);
  EXPECT_EQ(logos[0].Width(), 208U);
  EXPECT_EQ(logos[0].Height(), 112U);
  EXPECT_EQ(logos[3].Width(), 320U);
  EXPECT_EQ(Byte(Shipped("KOEILOGO.FNT"), 0x1AD0), 0x1A);
  EXPECT_EQ(Shipped("KOEILOGO.FNT").size(), 8465U);
}

TEST(ShippedEndingFont, Holds218GlyphsInBothCopies) {
  auto const glyphs{ DecodeGlyphs(Shipped("EMSG.16P")) };
  ASSERT_TRUE(glyphs);
  EXPECT_EQ(glyphs->size(), ENDING_GLYPHS);
  EXPECT_TRUE(std::ranges::equal(Shipped("EMSG"), Shipped("EMSG.16P")));
  EXPECT_EQ(EndingGlyphCodes()[0], 0x9D93);
  EXPECT_EQ(ImagesOf("EMSG.16P").size(), ENDING_GLYPHS);
}

TEST(ShippedGameFont, Holds1080DistinctGlyphsInBothCopies) {
  auto glyphs{ DecodeGlyphs(Shipped("MSG.16P")).value() };
  EXPECT_EQ(glyphs.size(), GAME_GLYPHS);
  EXPECT_TRUE(std::ranges::equal(Shipped("MSG"), Shipped("MSG.16P")));
  auto const records{ Chunks(Shipped("MSG.16P"), GLYPH_BYTES)
                      | std::views::transform([](Bytes const glyph) { return glyph | std::ranges::to<std::vector>(); })
                      | std::ranges::to<std::vector>() };
  auto sorted{ records };
  std::ranges::sort(sorted);
  EXPECT_EQ(std::ranges::unique(sorted).begin(), sorted.end());
}

TEST(ShippedCatalogue, DecodesEveryAssetOfEveryFile) {
  auto const catalogue{ Catalogue{ Embedded() } };
  EXPECT_EQ(catalogue.Assets().size(), 3'098U);
  EXPECT_TRUE(
      std::ranges::all_of(catalogue.Assets(), [&](Asset const& asset) { return catalogue.Decode(asset).has_value(); }));
  auto ids{ catalogue.Assets() | std::views::transform(&Asset::id) | std::ranges::to<std::vector>() };
  std::ranges::sort(ids);
  EXPECT_EQ(std::ranges::adjacent_find(ids), ids.end());
  EXPECT_TRUE(catalogue.Find("portrait/119"));
  EXPECT_FALSE(catalogue.Find("portrait/120"));
}

}
