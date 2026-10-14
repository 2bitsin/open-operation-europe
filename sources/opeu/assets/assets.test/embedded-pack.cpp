// The packs compiled into the executable, and a remaster laid over the classic pack: the remaster-sample (two images
// at 4x and 2x, its pack.json written by tools/assets/pack.py --check-remaster) over the checked-in classic pack.
#include "packs.hpp"

#include "opeu/assets/embedded-pack.hpp"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace opeu::assets {

namespace {

using test::CLASSIC_DIRECTORY;
using test::ClassicEmbedded;
using test::ToBytes;

constexpr auto REMASTER_SAMPLE{ std::string_view{ "assets.test/remaster-sample" } };

auto Layered(std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>> {
  return OpenLayered(std::make_unique<DirectoryPackFiles const>(CLASSIC_DIRECTORY), std::move(remaster));
}

auto RemasteredSample() -> std::unique_ptr<AssetPack const> {
  auto pack{ Layered(std::make_unique<DirectoryPackFiles const>(REMASTER_SAMPLE)) };
  EXPECT_TRUE(pack) << Describe(pack.error());
  return *std::move(pack);
}

}

TEST(EmbeddedPack, IsTheCheckedInClassicPack) {
  auto const embedded{ ClassicEmbedded() };
  auto const directory{ test::ClassicDirectory() };
  EXPECT_EQ(embedded.Ids(), directory.Ids());
  EXPECT_EQ(embedded.Load("silhouette/012"), directory.Load("silhouette/012"));
}

TEST(EmbeddedPack, GivesTheGameTheClassicPackWhenNoRemasterIsEmbedded) {
  auto const game{ OpenGamePack() };
  ASSERT_TRUE(game) << Describe(game.error());
  EXPECT_EQ((*game)->Ids().size(), 3'262U);
  EXPECT_EQ((*game)->Load("portrait/118")->texture.Width(), 64U);
}

TEST(RemasterOverClassic, DrawsAReplacedImageAtItsOwnScale) {
  auto const pack{ RemasteredSample() };
  auto const portrait{ pack->Load("portrait/118") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.scale, 4U);
  EXPECT_EQ(portrait->texture.Width(), 64U * 4U);
  EXPECT_EQ(portrait->texture.Height(), 80U * 4U);
  auto const yes{ pack->Load("dialog/yes") };
  ASSERT_TRUE(yes) << Describe(yes.error());
  EXPECT_EQ(yes->entry.scale, 2U);
  EXPECT_EQ(yes->texture.Width(), 32U * 2U);
}

TEST(RemasterOverClassic, DrawsEveryOtherImageFromTheClassicPack) {
  auto const pack{ RemasteredSample() };
  EXPECT_EQ(pack->Ids().size(), 3'262U);
  EXPECT_EQ(pack->Load("portrait/117"), ClassicEmbedded().Load("portrait/117"));
  EXPECT_EQ(pack->Palette("main/day"), ClassicEmbedded().Palette("main/day"));
  EXPECT_EQ(pack->FontNamed("text-glyph"), ClassicEmbedded().FontNamed("text-glyph"));
}

TEST(RemasterOverClassic, IsTheClassicPackAloneWithoutAManifest) {
  auto const pack{ Layered(std::make_unique<MemoryPackFiles const>(MemoryPackFiles::Files{})) };
  ASSERT_TRUE(pack);
  EXPECT_EQ((*pack)->Load("portrait/118")->texture.Width(), 64U);
}

TEST(RemasterOverClassic, RefusesABrokenManifest) {
  auto const pack{
    Layered(std::make_unique<MemoryPackFiles const>(MemoryPackFiles::Files{ { "pack.json", ToBytes("not json") } }
     ))
  };
  ASSERT_FALSE(pack);
  EXPECT_EQ(pack.error().fault, PackFault::BAD_MANIFEST);
}

}
