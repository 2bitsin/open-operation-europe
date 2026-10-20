// A pack loaded from its directory form and its archive form, by id: the loader on
// the synthetic pack, then the checked-in classic pack's own forms and names.
#include "packs.hpp"
#include "present.hpp"

#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <format>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Present;

using test::CheckedInClassicTest;
using test::ClassicArchive;
using test::ClassicDirectory;
using test::OpenMemoryPack;
using test::SAMPLE_IDS;
using test::SyntheticArchive;
using test::SyntheticDirectory;
using test::SyntheticFile;
using test::ToBytes;

using ClassicPackForms = CheckedInClassicTest;

// The synthetic portrait: 8x8.
auto PortraitImage() -> std::vector<std::byte> {
  return SyntheticFile(Present(SyntheticDirectory().Find("portrait/1")).image);
}

// A one-asset pack whose entry says the image is width x height at the given scale.
auto PackStating(std::size_t const width, std::size_t const height, std::string_view const scale) -> Packed<Pack> {
  auto const manifest{ std::format(R"({{"format": "opeu-pack/1", "name": "x", "scale": 1, "palettes": {{}}, "assets": [
    {{"id": "face", "class": "portraits", "role": "portrait", "width": {}, "height": {}, {} "palette": "grey", "palettes": ["grey"], "mask": false, "image": "face.png"}}]}})",
                                   width, height, scale) };
  return OpenMemoryPack({
      { "pack.json", ToBytes(manifest) },
      { "face.png",  PortraitImage()   }
  });
}

}

TEST(Pack, LoadsTheSameFromItsDirectoryAndItsArchive) {
  auto const directory{ SyntheticDirectory() };
  auto const archive{ SyntheticArchive() };
  ASSERT_EQ(directory.Ids(), archive.Ids());
  EXPECT_EQ(directory.Ids().size(), 7U);
  EXPECT_TRUE(std::ranges::all_of(directory.Ids(), [&](std::string const& id) {
    auto const loaded{ directory.Load(id) };
    return loaded && loaded == archive.Load(id);
  }));
}

TEST(Pack, LoadsAnAssetByItsId) {
  auto const portrait{ SyntheticDirectory().Load("portrait/1") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.role, "portrait");
  EXPECT_EQ(portrait->texture.Width(), 8U);
  EXPECT_EQ(portrait->texture.Height(), 8U);
  EXPECT_EQ(portrait->texture.Pixels().size(), 8U * 8U * 4U);
}

TEST(Pack, GivesTheIndexMapExactlyWhereThePackRecolours) {
  auto const pack{ SyntheticDirectory() };
  EXPECT_TRUE(std::ranges::all_of(pack.Ids(), [&](std::string const& id) {
    auto const loaded{ pack.Load(id) };
    return loaded && loaded->indexed.has_value() == loaded->entry.recolour.has_value();
  }));
  EXPECT_EQ(pack.Load("tile/0").value().entry.recolour, "hour");
  EXPECT_EQ(pack.Load("glyph/065").value().entry.recolour, "ink");
  EXPECT_FALSE(pack.Load("portrait/1").value().indexed);
}

TEST(Pack, ListsASideZeroVariant) {
  auto const variant{ SyntheticDirectory().Find("sprite/0/side-0") };
  ASSERT_TRUE(variant);
  EXPECT_EQ(Present(variant).variant_of, "sprite/0");
  EXPECT_EQ(Present(variant).variant, "side-0");
}

TEST(Pack, KnowsAnimationsAndPalettes) {
  auto const pack{ SyntheticDirectory() };
  EXPECT_EQ(pack.Frames("blink"), (std::vector<std::string>{ "sprite/0", "sprite/1" }));
  EXPECT_FALSE(pack.Frames("wink"));
  EXPECT_EQ(Present(pack.Palette("grey"))[3], (Colour{ .r = 48, .g = 48, .b = 48 }));
  EXPECT_TRUE(pack.Palette("night"));
  EXPECT_FALSE(pack.Palette("main/noon"));
}

TEST(Pack, KnowsItsFontsAndItsScale) {
  auto const pack{ SyntheticDirectory() };
  EXPECT_EQ(pack.FontNames(), std::vector<std::string>{ "glyphs" });
  auto const glyphs{ pack.FontNamed("glyphs") };
  ASSERT_TRUE(glyphs);
  EXPECT_EQ(Present(glyphs).cell_width, 4U);
  EXPECT_EQ(Present(glyphs).cell_height, 4U);
  EXPECT_EQ(Present(glyphs).first_code, 65U);
  EXPECT_EQ(Present(glyphs).glyphs, (std::vector<std::string>{ "glyph/065", "glyph/066" }));
  EXPECT_FALSE(pack.FontNamed("kana"));
  EXPECT_EQ(pack.Scale(), 1U);
}

TEST(Pack, SaysAnUnknownIdIsUnknown) {
  auto const pack{ SyntheticDirectory() };
  EXPECT_FALSE(pack.Find("portrait/120"));
  EXPECT_FALSE(pack.Lists("portrait/120"));
  EXPECT_EQ(pack.Load("portrait/120").error().fault, PackFault::UNKNOWN_ASSET);
}

TEST(Pack, RefusesAnImageOfAnotherSize) {
  ASSERT_TRUE(PackStating(8, 8, "")->Load("face"));
  EXPECT_EQ(PackStating(8, 9, "")->Load("face").error().fault, PackFault::BAD_IMAGE);
}

TEST(Pack, ReadsAnEntrysOwnScale) {
  ASSERT_TRUE(PackStating(4, 4, R"("scale": 2,)")->Load("face"));
  EXPECT_EQ(Present(PackStating(4, 4, R"("scale": 2,)")->Find("face")).scale, 2U);
  EXPECT_EQ(PackStating(8, 8, R"("scale": 2,)")->Load("face").error().fault, PackFault::BAD_IMAGE);
  EXPECT_EQ(PackStating(8, 8, R"("scale": 0,)").error().fault, PackFault::BAD_MANIFEST);
}

TEST(Pack, RefusesAFileThatIsNotAPng) {
  auto const portrait{ Present(SyntheticDirectory().Find("portrait/1")).image };
  auto const pack{
    OpenMemoryPack({ { "pack.json", SyntheticFile("pack.json") }, { portrait, ToBytes("GIF89a") } }
    )
  };
  ASSERT_TRUE(pack);
  EXPECT_EQ(pack->Load("portrait/1").error().fault, PackFault::BAD_IMAGE);
  EXPECT_EQ(pack->Load("tile/0").error().fault, PackFault::MISSING_FILE);
}

TEST(Pack, NeedsAManifest) {
  EXPECT_EQ(OpenMemoryPack({}).error().fault, PackFault::MISSING_FILE);
  EXPECT_EQ(OpenPack("assets.test/no-such.opeupack").error().fault, PackFault::MISSING_FILE);
}

TEST_F(ClassicPackForms, LoadTheSameFromTheDirectoryAndAnArchive) {
  auto const directory{ ClassicDirectory() };
  auto const archive{ ClassicArchive() };
  ASSERT_EQ(directory.Ids(), archive.Ids());
  EXPECT_EQ(directory.Ids().size(), 3'262U);
  EXPECT_TRUE(std::ranges::all_of(SAMPLE_IDS, [&](std::string_view const id) {
    auto const loaded{ directory.Load(id) };
    return loaded && loaded == archive.Load(id) && loaded->indexed.has_value() == loaded->entry.recolour.has_value();
  }));
  EXPECT_EQ(directory.Load("opening/00").value().entry.recolour, "scene");
}

TEST_F(ClassicPackForms, NameEachImageForWhatItShows) {
  auto const pack{ ClassicDirectory() };
  EXPECT_EQ(Present(pack.Find("portrait/005")).image, "portraits/portrait-005-busch.png");
  EXPECT_EQ(Present(pack.Find("silhouette/012")).image, "silhouettes/silhouette-012-pzkpfw-i.png");
  EXPECT_EQ(Present(pack.Find("event/27")).image, "pictures/event-27-pontoon-crossing.png");
  EXPECT_EQ(Present(pack.Find("battle-unit/class-03/facing-1/side-0")).image, "battle/battle-unit-class-03-facing-1-light-tank-side-0.png");
  EXPECT_EQ(Present(pack.Find("event/27")).description, "event picture: a tank crossing a pontoon bridge");
  EXPECT_EQ(Present(pack.Find("portrait/005")).scale, 1U);
}

TEST_F(ClassicPackForms, HoldTheGamesFontsAnimationsAndPalettes) {
  auto const pack{ ClassicDirectory() };
  EXPECT_EQ(pack.FontNames(), (std::vector<std::string>{ "digit-glyph", "ending-glyph", "game-glyph", "marker-glyph", "sideways-digit-glyph", "text-glyph" }));
  auto const text{ pack.FontNamed("text-glyph") };
  ASSERT_TRUE(text);
  EXPECT_EQ(Present(text).first_code, 0x20U);
  EXPECT_EQ(Present(text).glyphs.size(), 96U);
  EXPECT_EQ(Present(text).glyphs[0x41 - 0x20], "text-glyph/065");
  EXPECT_EQ(Present(pack.FontNamed("digit-glyph")).cell_width, 6U);
  EXPECT_EQ(pack.Frames("battle-blast/large"), (std::vector<std::string>{ "battle-blast/large/0", "battle-blast/large/1", "battle-blast/large/2", "battle-blast/large/3" }));
  EXPECT_EQ(pack.Palette("main/day"), Colours(Present(FindPalette("main/day")).levels));
}

}
