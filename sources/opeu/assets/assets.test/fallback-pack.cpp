// A remaster pack over the classic one: the remaster answers for its ids, the classic pack for the rest.
#include "packs.hpp"

#include "opeu/assets/fallback-pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <string>

namespace opeu::assets {

namespace {

using test::OpenMemoryPack;
using test::OpenSample;
using test::SAMPLE_ARCHIVE;
using test::SampleFile;
using test::ToBytes;

// portrait/118 replaced by a 32x16 picture, an asset the classic pack lacks, a palette of its own, and opening/00
// listed with an image the pack does not hold.
constexpr auto REMASTER{ std::string_view{ R"({
 "format": "opeu-pack/1", "name": "remaster", "scale": 1,
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
    OpenMemoryPack({ { "pack.json", ToBytes(REMASTER) }, { "a.png", SampleFile("dialogs/dialog/yes.png") } }
    )
  };
  EXPECT_TRUE(remaster);
  return FallbackPack{ std::make_unique<Pack const>(*std::move(remaster)), std::make_unique<Pack const>(OpenSample(SAMPLE_ARCHIVE)) };
}

}

TEST(FallbackPack, TakesAnAssetFromThePreferredPackWhenItHasIt) {
  auto const portrait{ Remastered().Load("portrait/118") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.palette, "sepia");
  EXPECT_EQ(portrait->texture, OpenSample(SAMPLE_ARCHIVE).Load("dialog/yes")->texture);
}

TEST(FallbackPack, FallsBackForEveryOtherAsset) {
  auto const pack{ Remastered() };
  auto const classic{ OpenSample(SAMPLE_ARCHIVE) };
  EXPECT_EQ(pack.Load("dialog/yes"), classic.Load("dialog/yes"));
  EXPECT_EQ(pack.Find("silhouette/012"), classic.Find("silhouette/012"));
  EXPECT_EQ(pack.Load("portrait/117").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(FallbackPack, ReportsABrokenReplacementRatherThanHidingIt) {
  EXPECT_EQ(Remastered().Load("opening/00").error().fault, PackFault::MISSING_FILE);
}

TEST(FallbackPack, ListsTheIdsOfBothPacks) {
  auto const ids{ Remastered().Ids() };
  EXPECT_EQ(ids.size(), 19U);
  EXPECT_TRUE(std::ranges::is_sorted(ids));
  EXPECT_TRUE(std::ranges::contains(ids, "portrait/200"));
  EXPECT_TRUE(std::ranges::contains(ids, "dialog/yes"));
}

TEST(FallbackPack, FindsPalettesInEitherPack) {
  auto const pack{ Remastered() };
  EXPECT_EQ((*pack.Palette("sepia"))[0], (Colour{ .r = 1, .g = 1, .b = 1 }));
  EXPECT_EQ(pack.Palette("main/day"), Colours(FindPalette("main/day")->levels));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

}
