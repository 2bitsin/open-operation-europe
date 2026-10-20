// A remaster pack over the classic one: the remaster answers for its ids, the classic pack for the rest.
#include "packs.hpp"
#include "present.hpp"

#include "opeu/assets/fallback-pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Present;

using test::OpenMemoryPack;
using test::SyntheticDirectory;
using test::SyntheticFile;
using test::ToBytes;

// The synthetic tile's 8x8 image.
auto TileImage() -> std::vector<std::byte> {
  return SyntheticFile(Present(SyntheticDirectory().Find("tile/0")).image);
}

// portrait/1 replaced by the tile's picture, an asset the synthetic pack lacks, a palette of its own, and sprite/0
// listed with an image the pack does not hold.
constexpr auto REMASTER{ std::string_view{ R"({
 "format": "opeu-pack/1", "name": "remaster", "scale": 1,
 "animations": {"blink": ["portrait/1"], "dance": ["portrait/200"]},
 "fonts": {"glyphs": {"cell": [8, 8], "first": 32, "glyphs": ["portrait/1"]}, "runes": {"cell": [8, 8], "glyphs": ["portrait/200"]}},
 "palettes": {"sepia": [[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [5, 5, 5], [6, 6, 6], [7, 7, 7], [8, 8, 8],
                        [9, 9, 9], [10, 10, 10], [11, 11, 11], [12, 12, 12], [13, 13, 13], [14, 14, 14], [15, 15, 15], [16, 16, 16]]},
 "assets": [
  {"id": "portrait/1", "class": "portraits", "role": "portrait", "width": 8, "height": 8, "palette": "sepia", "palettes": ["sepia"], "mask": false, "image": "a.png"},
  {"id": "portrait/200", "class": "portraits", "role": "portrait", "width": 8, "height": 8, "palette": "sepia", "palettes": ["sepia"], "mask": false, "image": "a.png"},
  {"id": "sprite/0", "class": "sprites", "role": "sprite", "width": 4, "height": 4, "palette": "sepia", "palettes": ["sepia"], "mask": true, "image": "gone.png"}
 ]
})" } };

auto Remastered() -> FallbackPack {
  auto remaster{
    OpenMemoryPack({ { "pack.json", ToBytes(REMASTER) }, { "a.png", TileImage() } }
    )
  };
  EXPECT_TRUE(remaster);
  return FallbackPack{ std::make_unique<Pack const>(*std::move(remaster)), std::make_unique<Pack const>(SyntheticDirectory()) };
}

}

TEST(FallbackPack, TakesAnAssetFromThePreferredPackWhenItHasIt) {
  auto const portrait{ Remastered().Load("portrait/1") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.palette, "sepia");
  EXPECT_EQ(portrait->texture, SyntheticDirectory().Load("tile/0").value().texture);
}

TEST(FallbackPack, FallsBackForEveryOtherAsset) {
  auto const pack{ Remastered() };
  auto const synthetic{ SyntheticDirectory() };
  EXPECT_EQ(pack.Load("tile/0"), synthetic.Load("tile/0"));
  EXPECT_EQ(pack.Find("glyph/065"), synthetic.Find("glyph/065"));
  EXPECT_EQ(pack.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(FallbackPack, ReportsABrokenReplacementRatherThanHidingIt) {
  EXPECT_EQ(Remastered().Load("sprite/0").error().fault, PackFault::MISSING_FILE);
}

TEST(FallbackPack, ListsTheIdsOfBothPacks) {
  auto const ids{ Remastered().Ids() };
  EXPECT_EQ(ids.size(), 8U);
  EXPECT_TRUE(std::ranges::is_sorted(ids));
  EXPECT_TRUE(std::ranges::contains(ids, "portrait/200"));
  EXPECT_TRUE(std::ranges::contains(ids, "tile/0"));
}

TEST(FallbackPack, ListsAnIdEitherPackHolds) {
  auto const pack{ Remastered() };
  EXPECT_TRUE(pack.Lists("portrait/200"));
  EXPECT_TRUE(pack.Lists("tile/0"));
  EXPECT_FALSE(pack.Lists("portrait/999"));
}

TEST(FallbackPack, TakesAnimationsAndFontsByNameFromThePreferredPackFirst) {
  auto const pack{ Remastered() };
  EXPECT_EQ(pack.Frames("blink"), std::vector<std::string>{ "portrait/1" });
  EXPECT_EQ(pack.Frames("dance"), std::vector<std::string>{ "portrait/200" });
  EXPECT_EQ(pack.Frames("wink"), std::nullopt);
  EXPECT_EQ(Present(pack.FontNamed("glyphs")).first_code, 32U);
  EXPECT_EQ(pack.FontNames(), (std::vector<std::string>{ "glyphs", "runes" }));
}

TEST(FallbackPack, HasThePreferredPacksScale) {
  auto const manifest{ std::string{ R"({"format": "opeu-pack/1", "name": "big", "scale": 4, "palettes": {}, "assets": []})" } };
  auto       big{
    OpenMemoryPack({ { "pack.json", ToBytes(manifest) } }
    )
  };
  ASSERT_TRUE(big);
  auto const pack{
    FallbackPack{ std::make_unique<Pack const>(*std::move(big)), std::make_unique<Pack const>(SyntheticDirectory()) }
  };
  EXPECT_EQ(pack.Scale(), 4U);
  EXPECT_EQ(Remastered().Scale(), 1U);
}

TEST(FallbackPack, FindsPalettesInEitherPack) {
  auto const pack{ Remastered() };
  EXPECT_EQ(Present(pack.Palette("sepia"))[0], (Colour{ .r = 1, .g = 1, .b = 1 }));
  EXPECT_EQ(pack.Palette("grey"), SyntheticDirectory().Palette("grey"));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

}
