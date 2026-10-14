// A remaster pack over the classic one: the remaster answers for its ids, the classic pack for the rest.
#include "packs.hpp"

#include "opeu/assets/fallback-pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::ClassicEmbedded;
using test::ClassicFile;
using test::OpenMemoryPack;
using test::ToBytes;

auto YesImage() -> std::vector<std::byte> {
  return ClassicFile(ClassicEmbedded().Find("dialog/yes")->image);
}

// portrait/118 replaced by a 32x16 picture, an asset the classic pack lacks, a palette of its own, and opening/00
// listed with an image the pack does not hold.
constexpr auto REMASTER{ std::string_view{ R"({
 "format": "opeu-pack/1", "name": "remaster", "scale": 1,
 "animations": {"battle-blast/large": ["portrait/118"], "blink": ["portrait/200"]},
 "fonts": {"text-glyph": {"cell": [8, 16], "first": 32, "glyphs": ["portrait/118"]}, "runes": {"cell": [32, 16], "glyphs": ["portrait/200"]}},
 "palettes": {"sepia": [[1, 1, 1], [2, 2, 2], [3, 3, 3], [4, 4, 4], [5, 5, 5], [6, 6, 6], [7, 7, 7], [8, 8, 8],
                        [9, 9, 9], [10, 10, 10], [11, 11, 11], [12, 12, 12], [13, 13, 13], [14, 14, 14], [15, 15, 15], [16, 16, 16]]},
 "assets": [
  {"id": "portrait/118", "class": "portraits", "role": "portrait", "width": 32, "height": 16, "palette": "sepia", "palettes": ["sepia"], "mask": false, "image": "a.png"},
  {"id": "portrait/200", "class": "portraits", "role": "portrait", "width": 32, "height": 16, "palette": "sepia", "palettes": ["sepia"], "mask": false, "image": "a.png"},
  {"id": "opening/00", "class": "pictures", "role": "picture", "width": 640, "height": 400, "palette": "sepia", "palettes": ["sepia"], "mask": false, "image": "gone.png"}
 ]
})" } };

auto Remastered() -> FallbackPack {
  auto remaster{
    OpenMemoryPack({ { "pack.json", ToBytes(REMASTER) }, { "a.png", YesImage() } }
    )
  };
  EXPECT_TRUE(remaster);
  return FallbackPack{ std::make_unique<Pack const>(*std::move(remaster)), std::make_unique<Pack const>(ClassicEmbedded()) };
}

}

TEST(FallbackPack, TakesAnAssetFromThePreferredPackWhenItHasIt) {
  auto const portrait{ Remastered().Load("portrait/118") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.palette, "sepia");
  EXPECT_EQ(portrait->texture, ClassicEmbedded().Load("dialog/yes")->texture);
}

TEST(FallbackPack, FallsBackForEveryOtherAsset) {
  auto const pack{ Remastered() };
  auto const classic{ ClassicEmbedded() };
  EXPECT_EQ(pack.Load("dialog/yes"), classic.Load("dialog/yes"));
  EXPECT_EQ(pack.Find("silhouette/012"), classic.Find("silhouette/012"));
  EXPECT_EQ(pack.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(FallbackPack, ReportsABrokenReplacementRatherThanHidingIt) {
  EXPECT_EQ(Remastered().Load("opening/00").error().fault, PackFault::MISSING_FILE);
}

TEST(FallbackPack, ListsTheIdsOfBothPacks) {
  auto const ids{ Remastered().Ids() };
  EXPECT_EQ(ids.size(), 3'263U);
  EXPECT_TRUE(std::ranges::is_sorted(ids));
  EXPECT_TRUE(std::ranges::contains(ids, "portrait/200"));
  EXPECT_TRUE(std::ranges::contains(ids, "dialog/yes"));
}

TEST(FallbackPack, TakesAnimationsAndFontsByNameFromThePreferredPackFirst) {
  auto const pack{ Remastered() };
  EXPECT_EQ(pack.Frames("battle-blast/large"), std::vector<std::string>{ "portrait/118" });
  EXPECT_EQ(pack.Frames("blink"), std::vector<std::string>{ "portrait/200" });
  EXPECT_EQ(pack.Frames("battle-blast/small"), ClassicEmbedded().Frames("battle-blast/small"));
  EXPECT_EQ(pack.Frames("battle-blast/huge"), std::nullopt);
  EXPECT_EQ(pack.FontNamed("text-glyph")->first_code, 32U);
  EXPECT_EQ(pack.FontNamed("ending-glyph"), ClassicEmbedded().FontNamed("ending-glyph"));
  EXPECT_EQ(pack.FontNames(), (std::vector<std::string>{ "digit-glyph", "ending-glyph", "game-glyph", "marker-glyph", "runes", "sideways-digit-glyph", "text-glyph" }));
}

TEST(FallbackPack, HasThePreferredPacksScale) {
  auto const manifest{ std::string{ R"({"format": "opeu-pack/1", "name": "big", "scale": 4, "palettes": {}, "assets": []})" } };
  auto       big{
    OpenMemoryPack({ { "pack.json", ToBytes(manifest) } }
    )
  };
  ASSERT_TRUE(big);
  auto const pack{
    FallbackPack{ std::make_unique<Pack const>(*std::move(big)), std::make_unique<Pack const>(ClassicEmbedded()) }
  };
  EXPECT_EQ(pack.Scale(), 4U);
  EXPECT_EQ(Remastered().Scale(), 1U);
}

TEST(FallbackPack, FindsPalettesInEitherPack) {
  auto const pack{ Remastered() };
  EXPECT_EQ((*pack.Palette("sepia"))[0], (Colour{ .r = 1, .g = 1, .b = 1 }));
  EXPECT_EQ(pack.Palette("main/day"), Colours(FindPalette("main/day")->levels));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

}
