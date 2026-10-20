// The raw bitmap files: tiles, battle sprites, portraits, silhouettes, dialogs, masks, logos and glyph fonts.
#include "present.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/glyphs.hpp"
#include "opeu/assets/runs.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <string_view>
#include <vector>

namespace opeu::assets {

namespace {

using test::Present;

using ShippedBattleTerrain = test::ShippedTest;
using ShippedBattleUnits   = test::ShippedTest;
using ShippedCatalogue     = test::ShippedTest;
using ShippedDialogs       = test::ShippedTest;
using ShippedEndingFont    = test::ShippedTest;
using ShippedEndingMasks   = test::ShippedTest;
using ShippedGameFont      = test::ShippedTest;
using ShippedLogo          = test::ShippedTest;
using ShippedOpeningMasks  = test::ShippedTest;
using ShippedOverviewTiles = test::ShippedTest;
using ShippedPortraits     = test::ShippedTest;
using ShippedSilhouettes   = test::ShippedTest;
using ShippedViewTiles     = test::ShippedTest;

// Every asset of a file decoded; each must decode.
auto ImagesOf(Catalogue const& catalogue, std::string_view const file) -> std::vector<Image> {
  return catalogue.Assets() | std::views::filter([file](Asset const& asset) { return asset.file == file; }) | std::views::transform([&catalogue](Asset const& asset) {
           auto const image{ catalogue.Decode(asset) };
           EXPECT_TRUE(image) << asset.id;
           return image.value_or(Image{});
         })
         | std::ranges::to<std::vector>();
}

auto AllSized(std::vector<Image> const& images, std::size_t const width, std::size_t const height) -> bool {
  return std::ranges::all_of(images, [&](Image const& image) { return image.Width() == width && image.Height() == height; });
}

auto Highest(Image const& image) -> std::uint8_t {
  return std::ranges::max(image.Pixels());
}

// One glyph's bytes as a fixed-size record, so sorting compares a known length.
using GlyphRecord = std::array<std::byte, GLYPH_BYTES>;

auto ToRecord(Bytes const glyph) -> GlyphRecord {
  GlyphRecord record{};
  std::ranges::copy(glyph, record.begin());
  return record;
}

}

TEST_F(ShippedOverviewTiles, HoldNinetySixTilesPerMap) {
  auto const tiles{ ImagesOf(Catalogued(), "BIGMAP.DAT") };
  EXPECT_EQ(tiles.size(), 6U * 96U);
  EXPECT_TRUE(AllSized(tiles, 8, 8));
  EXPECT_EQ(Shipped("BIGMAP.DAT").size(), tiles.size() * 32);
}

TEST_F(ShippedViewTiles, HoldNinetySixTilesPerMap) {
  auto const tiles{ ImagesOf(Catalogued(), "MIDMAP.DAT") };
  EXPECT_EQ(tiles.size(), 6U * 96U);
  EXPECT_TRUE(AllSized(tiles, 24, 24));
  EXPECT_EQ(Shipped("MIDMAP.DAT").size(), tiles.size() * 288);
}

TEST_F(ShippedBattleTerrain, HoldNinetyFiveMaskedCellsPerSeason) {
  auto const summer{ ImagesOf(Catalogued(), "HEXFONT.DAT") };
  auto const snow{ ImagesOf(Catalogued(), "HEXFONTS.DAT") };
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

TEST_F(ShippedBattleUnits, HoldIconsBlastsAndTwoBorders) {
  auto const sprites{ ImagesOf(Catalogued(), "HEXUNIT.DAT") };
  ASSERT_EQ(sprites.size(), 54U);
  EXPECT_TRUE(AllSized(sprites | std::views::take(52) | std::ranges::to<std::vector>(), 40, 26));
  EXPECT_TRUE(AllSized(sprites | std::views::drop(52) | std::ranges::to<std::vector>(), 24, 20));
  EXPECT_EQ(Shipped("HEXUNIT.DAT").size(), (52U * 650U) + (2U * 300U));
  auto const swapped{ SideZeroColours(sprites.front()) };
  EXPECT_NE(swapped, sprites.front());
  EXPECT_EQ(SideZeroColours(swapped), sprites.front());
}

TEST_F(ShippedPortraits, HoldOneHundredTwentyEightColourFaces) {
  auto const faces{ ImagesOf(Catalogued(), "FACE.DAT") };
  ASSERT_EQ(faces.size(), 120U);
  EXPECT_TRUE(AllSized(faces, 64, 80));
  EXPECT_TRUE(std::ranges::all_of(faces, [](Image const& face) { return Highest(face) < 8; }));
  EXPECT_EQ(Shipped("FACE.DAT").size(), 120U * 1920U);
}

TEST_F(ShippedSilhouettes, HoldOneHundredFiftyNineRunPackedRecords) {
  auto const& catalogue{ Catalogued() };
  auto const  records{ catalogue.Assets() | std::views::filter([](Asset const& a) { return a.file == "ARMS.SIL"; }) | std::ranges::to<std::vector>() };
  ASSERT_EQ(records.size(), 159U);
  auto const contents{ Shipped("ARMS.SIL") };
  auto const bytes{ Bytes{ contents } };
  auto const ends{ std::views::zip(records, records | std::views::drop(1)) };
  EXPECT_TRUE(std::ranges::all_of(ends, [bytes](auto const& pair) {
    auto const& [record, next]{ pair };
    auto const expanded{ ExpandRuns(bytes.subspan(record.at), 0x780) };
    return expanded && record.at + expanded->consumed <= next.at;
  }));
  EXPECT_EQ(records.back().at + ExpandRuns(bytes.subspan(records.back().at), 0x780)->consumed, 121'137U);
  EXPECT_TRUE(AllSized(ImagesOf(Catalogued(), "ARMS.SIL"), 192, 80));
}

TEST_F(ShippedDialogs, HoldTheNumberDialogFourButtonsAndThePanel) {
  auto const bitmaps{ ImagesOf(Catalogued(), "MSGRP.DAT") };
  ASSERT_EQ(bitmaps.size(), 6U);
  EXPECT_EQ(bitmaps[0].Width(), 144U);
  EXPECT_EQ(bitmaps[0].Height(), 96U);
  EXPECT_TRUE(AllSized(bitmaps | std::views::drop(1) | std::views::take(4) | std::ranges::to<std::vector>(), 32, 16));
  EXPECT_EQ(bitmaps[5].Width(), 256U);
  EXPECT_EQ(bitmaps[5].Height(), 120U);
  EXPECT_EQ(Shipped("MSGRP.DAT").size(), 0x5B00U);
}

TEST_F(ShippedOpeningMasks, HoldSixBlastMasksAndSixShells) {
  auto const masks{ ImagesOf(Catalogued(), "MOPENGRP.DAT") };
  ASSERT_EQ(masks.size(), 12U);
  EXPECT_TRUE(AllSized(masks | std::views::take(6) | std::ranges::to<std::vector>(), 208, 176));
  EXPECT_EQ(masks[6].Width(), 64U);
  EXPECT_EQ(masks[6].Height(), 40U);
  EXPECT_EQ(Shipped("MOPENGRP.DAT").size(), (6U * 4576U) + 512U);
}

TEST_F(ShippedEndingMasks, HoldEightAircraftFrames) {
  auto const masks{ ImagesOf(Catalogued(), "MENDGRP.DAT") };
  ASSERT_EQ(masks.size(), 8U);
  EXPECT_TRUE(AllSized(masks, 256, 64));
  EXPECT_EQ(Shipped("MENDGRP.DAT").size(), 8U * 2048U);
}

TEST_F(ShippedLogo, HoldsFourOneBitCaptionsAndAnEndOfFileMark) {
  auto const logos{ ImagesOf(Catalogued(), "KOEILOGO.FNT") };
  ASSERT_EQ(logos.size(), 4U);
  EXPECT_EQ(logos[0].Width(), 208U);
  EXPECT_EQ(logos[0].Height(), 112U);
  EXPECT_EQ(logos[3].Width(), 320U);
  EXPECT_EQ(Byte(Shipped("KOEILOGO.FNT"), 0x1AD0), 0x1A);
  EXPECT_EQ(Shipped("KOEILOGO.FNT").size(), 8465U);
}

TEST_F(ShippedEndingFont, Holds218GlyphsInBothCopies) {
  auto const glyphs{ DecodeGlyphs(Shipped("EMSG.16P")) };
  ASSERT_TRUE(glyphs);
  EXPECT_EQ(glyphs->size(), ENDING_GLYPHS);
  EXPECT_TRUE(std::ranges::equal(Shipped("EMSG"), Shipped("EMSG.16P")));
  EXPECT_EQ(EndingGlyphCodes()[0], 0x9D93);
  EXPECT_EQ(ImagesOf(Catalogued(), "EMSG.16P").size(), ENDING_GLYPHS);
}

TEST_F(ShippedGameFont, Holds1080DistinctGlyphsInBothCopies) {
  auto const glyphs{ DecodeGlyphs(Shipped("MSG.16P")).value() };
  EXPECT_EQ(glyphs.size(), GAME_GLYPHS);
  EXPECT_TRUE(std::ranges::equal(Shipped("MSG"), Shipped("MSG.16P")));
  auto const records{ Chunks(Shipped("MSG.16P"), GLYPH_BYTES) | std::views::transform(ToRecord) | std::ranges::to<std::vector>() };
  auto       sorted{ records };
  std::ranges::sort(sorted);
  EXPECT_EQ(std::ranges::unique(sorted).begin(), sorted.end());
}

TEST_F(ShippedCatalogue, DecodesEveryAssetOfEveryFile) {
  auto const& catalogue{ Catalogued() };
  EXPECT_EQ(catalogue.Assets().size(), 3'218U); // 3,098 from the shipped files, 120 from the program's glyph tables
  EXPECT_TRUE(std::ranges::all_of(catalogue.Assets(), [&](Asset const& asset) { return catalogue.Decode(asset).has_value(); }));
  auto ids{ catalogue.Assets() | std::views::transform(&Asset::id) | std::ranges::to<std::vector>() };
  std::ranges::sort(ids);
  EXPECT_EQ(std::ranges::adjacent_find(ids), ids.end());
  auto const texture{ catalogue.DecodeTexture(Present(catalogue.Find("portrait/119"))) };
  ASSERT_TRUE(texture);
  EXPECT_EQ(texture->Pitch(), 64U * 4U);
  EXPECT_EQ(texture->Pixels().size(), 64U * 80U * 4U);
  EXPECT_FALSE(catalogue.Find("portrait/120"));
}

}
