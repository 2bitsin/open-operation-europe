// A remaster laid over a base pack: the synthetic remaster (two images at 4x and 2x) over the synthetic pack, from its
// files or its path.
#include "packs.hpp"

#include "opeu/assets/layered-pack.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace opeu::assets {

namespace {
using namespace std::string_literals;
using namespace std::string_view_literals;
using test::SYNTHETIC_REMASTER;
using test::SyntheticDirectory;
using test::ToBytes;

auto Base() -> std::unique_ptr<AssetPack const> {
  return std::make_unique<Pack const>(SyntheticDirectory());
}

auto Layered(std::unique_ptr<PackFiles const> remaster) -> Packed<std::unique_ptr<AssetPack const>> {
  return OpenLayered(Base(), std::move(remaster));
}

auto Remastered() -> std::unique_ptr<AssetPack const> {
  auto pack{ Layered(std::make_unique<DirectoryPackFiles const>(SYNTHETIC_REMASTER)) };
  EXPECT_TRUE(pack) << Describe(pack.error());
  return *std::move(pack);
}

}

TEST(RemasterOverBase, DrawsAReplacedImageAtItsOwnScale) {
  auto const pack{ Remastered() };
  auto const portrait{ pack->Load("portrait/1") };
  ASSERT_TRUE(portrait) << Describe(portrait.error());
  EXPECT_EQ(portrait->entry.scale, 4U);
  EXPECT_EQ(portrait->texture.Width(), 8U * 4U);
  EXPECT_EQ(portrait->texture.Height(), 8U * 4U);
  auto const sprite{ pack->Load("sprite/1") };
  ASSERT_TRUE(sprite) << Describe(sprite.error());
  EXPECT_EQ(sprite->entry.scale, 2U);
  EXPECT_EQ(sprite->texture.Width(), 4U * 2U);
}

TEST(RemasterOverBase, DrawsEveryOtherImageFromTheBase) {
  auto const pack{ Remastered() };
  auto const base{ SyntheticDirectory() };
  EXPECT_EQ(pack->Ids(), base.Ids());
  EXPECT_EQ(pack->Load("tile/0"), base.Load("tile/0"));
  EXPECT_EQ(pack->Palette("grey"), base.Palette("grey"));
  EXPECT_EQ(pack->FontNamed("glyphs"), base.FontNamed("glyphs"));
}

TEST(RemasterOverBase, IsTheBaseAloneWithoutAManifest) {
  auto const pack{ Layered(std::make_unique<MemoryPackFiles const>(MemoryPackFiles::Files{})) };
  ASSERT_TRUE(pack);
  EXPECT_EQ((*pack)->Load("portrait/1").value().texture.Width(), 8U);
}

TEST(RemasterOverBase, IsTheBaseAloneWithoutARemasterPath) {
  auto const pack{ OpenLayered(Base(), std::optional<std::filesystem::path>{}) };
  ASSERT_TRUE(pack);
  EXPECT_EQ((*pack)->Scale(), 1U);
}

TEST(RemasterOverBase, OpensARemasterByPath) {
  auto const pack{ OpenLayered(Base(), std::optional{ std::filesystem::path{ SYNTHETIC_REMASTER } }) };
  ASSERT_TRUE(pack) << Describe(pack.error());
  EXPECT_EQ((*pack)->Load("portrait/1").value().entry.scale, 4U);
}

TEST(RemasterOverBase, RefusesARemasterPathWithoutAPack) {
  EXPECT_FALSE(OpenLayered(Base(), std::optional{ std::filesystem::path{ "no-such-remaster" } }));
}

TEST(RemasterOverBase, RefusesABrokenManifest) {
  auto const pack{
    Layered(std::make_unique<MemoryPackFiles const>(MemoryPackFiles::Files{ { "pack.json", ToBytes("not json") } }
     ))
  };
  ASSERT_FALSE(pack);
  EXPECT_EQ(pack.error().fault, PackFault::BAD_MANIFEST);
}

}
