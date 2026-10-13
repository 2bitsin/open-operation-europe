// pack.json: the entries, palettes, animations and fonts, and the manifests that are refused.
#include "packs.hpp"

#include "opeu/assets/manifest.hpp"

#include <gtest/gtest.h>

#include <string>
#include <string_view>

namespace opeu::assets {

namespace {

using test::SampleFile;
using test::ToBytes;

constexpr auto TINY{ std::string_view{ R"({
 "format": "opeu-pack/1", "name": "tiny", "scale": 4,
 "palettes": {"grey": [[0, 0, 0], [8, 8, 8], [16, 16, 16], [24, 24, 24], [32, 32, 32], [40, 40, 40], [48, 48, 48], [56, 56, 56],
                       [64, 64, 64], [72, 72, 72], [80, 80, 80], [88, 88, 88], [96, 96, 96], [104, 104, 104], [112, 112, 112], [120, 120, 120]]},
 "animations": {"blink": ["eye/1", "eye/0"]},
 "fonts": {"eyes": {"cell": [2, 3], "first": 48, "glyphs": ["eye/0", "eye/1"]}, "plain": {"cell": [2, 3], "glyphs": ["eye/1"]}},
 "assets": [
  {"id": "eye/0", "class": "sprites", "role": "sprite", "width": 2, "height": 3, "palette": "grey", "palettes": ["grey"], "mask": true, "image": "sprites/eye/0.png",
   "recolour": "ink", "indexed": "sprites/eye/0.index.png", "animation": "blink", "frame": 1, "use": "an eye"},
  {"id": "eye/1", "class": "sprites", "role": "sprite", "width": 2, "height": 3, "palette": "grey", "palettes": ["grey"], "mask": false, "image": "sprites/eye/1.png"}
 ]
})" } };

auto Fault(std::string_view const json) -> PackFault {
  auto const manifest{ Manifest::Parse(ToBytes(json)) };
  return manifest ? PackFault{} : manifest.error().fault;
}

}

TEST(Manifest, ReadsEveryFieldOfAnEntry) {
  auto const manifest{ Manifest::Parse(ToBytes(TINY)) };
  ASSERT_TRUE(manifest) << Describe(manifest.error());
  auto const eye{ manifest->Find("eye/0") };
  ASSERT_TRUE(eye);
  EXPECT_EQ(eye->asset_class, "sprites");
  EXPECT_EQ(eye->width, 2U);
  EXPECT_EQ(eye->height, 3U);
  EXPECT_TRUE(eye->mask);
  EXPECT_EQ(eye->recolour, "ink");
  EXPECT_EQ(eye->indexed, "sprites/eye/0.index.png");
  EXPECT_EQ(eye->animation, "blink");
  EXPECT_EQ(eye->frame, 1U);
  EXPECT_EQ(eye->use, "an eye");
}

TEST(Manifest, LeavesOptionalFieldsEmpty) {
  auto const eye{ Manifest::Parse(ToBytes(TINY))->Find("eye/1") };
  ASSERT_TRUE(eye);
  EXPECT_FALSE(eye->recolour);
  EXPECT_FALSE(eye->indexed);
  EXPECT_TRUE(eye->animation.empty());
  EXPECT_TRUE(eye->variant_of.empty());
}

TEST(Manifest, ListsIdsSortedAndKnowsTheScale) {
  auto const manifest{ Manifest::Parse(ToBytes(TINY)) };
  ASSERT_TRUE(manifest);
  EXPECT_EQ(manifest->Name(), "tiny");
  EXPECT_EQ(manifest->Scale(), 4U);
  EXPECT_EQ(manifest->Ids(), (std::vector<std::string>{ "eye/0", "eye/1" }));
  EXPECT_FALSE(manifest->Find("eye/2"));
}

TEST(Manifest, KeepsPalettesAndAnimations) {
  auto const manifest{ Manifest::Parse(ToBytes(TINY)) };
  ASSERT_TRUE(manifest);
  auto const grey{ manifest->Palette("grey") };
  ASSERT_TRUE(grey);
  EXPECT_EQ((*grey)[15], (Colour{ .r = 120, .g = 120, .b = 120 }));
  EXPECT_FALSE(manifest->Palette("sepia"));
  EXPECT_EQ(manifest->Frames("blink"), (std::vector<std::string>{ "eye/1", "eye/0" }));
}

TEST(Manifest, ListsItsFontsWithTheirFirstCode) {
  auto const manifest{ Manifest::Parse(ToBytes(TINY)) };
  ASSERT_TRUE(manifest);
  EXPECT_EQ(manifest->FontNames(), (std::vector<std::string>{ "eyes", "plain" }));
  EXPECT_EQ(manifest->FontNamed("eyes")->first_code, 48U);
  EXPECT_EQ(manifest->FontNamed("plain")->first_code, 0U); // no "first": the glyph numbers are the codes
}

TEST(Manifest, KeepsTheSamplesFonts) {
  auto const manifest{ Manifest::Parse(SampleFile("pack.json")) };
  ASSERT_TRUE(manifest);
  auto const font{ manifest->FontNamed("game-glyph") };
  ASSERT_TRUE(font);
  EXPECT_EQ(font->cell_width, 16U);
  EXPECT_EQ(font->cell_height, 14U);
  EXPECT_EQ(font->glyphs, std::vector<std::string>{ "game-glyph/0042" });
  EXPECT_EQ(manifest->Frames("battle-blast/large")->size(), 4U);
}

TEST(Manifest, RefusesWhatIsNotAManifest) {
  EXPECT_EQ(Fault("not json"), PackFault::BAD_MANIFEST);
  EXPECT_EQ(Fault(R"({"format": "opeu-pack/2", "name": "x", "scale": 1, "palettes": {}, "assets": []})"), PackFault::BAD_MANIFEST);
  EXPECT_EQ(Fault(R"({"format": "opeu-pack/1", "name": "x", "scale": 0, "palettes": {}, "assets": []})"), PackFault::BAD_MANIFEST);
  EXPECT_EQ(Fault(R"({"format": "opeu-pack/1", "name": "x", "scale": 1, "palettes": {}, "assets": [{"id": "a"}]})"), PackFault::BAD_MANIFEST);
  EXPECT_EQ(Fault(R"({"format": "opeu-pack/1", "name": "x", "scale": 1, "palettes": {}, "assets": [{"id": 3}]})"), PackFault::BAD_MANIFEST);
}

}
