// The classic pack against the decoders: every entry and image of the pack is what the decoders' pack (CataloguePack)
// draws from the shipped files, pixel for pixel. By default the sample in assets.test/packs/ in both its forms;
// tools/assets/pack.test runs this over the whole classic pack, naming it in OPEU_CLASSIC_PACK, and then the pack must
// also hold every id, animation and font the decoders know.
#include "packs.hpp"
#include "shipped.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/palette.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::Embedded;
using test::SAMPLE_ARCHIVE;
using test::SAMPLE_DIRECTORY;

auto PacksUnderTest() -> std::vector<std::filesystem::path> {
  auto const given{ std::getenv("OPEU_CLASSIC_PACK") };
  if (given == nullptr) return { SAMPLE_DIRECTORY, SAMPLE_ARCHIVE };
  return { given };
}

auto WholePackGiven() -> bool {
  return std::getenv("OPEU_CLASSIC_PACK") != nullptr;
}

// The whole pack lists what the decoders list: every id, every animation's frames, every font.
auto ExpectComplete(AssetPack const& pack, AssetPack const& decoders) -> void {
  EXPECT_EQ(pack.Ids(), decoders.Ids());
  EXPECT_EQ(pack.FontNames(), decoders.FontNames());
  std::ranges::for_each(decoders.FontNames(), [&](std::string const& name) { EXPECT_EQ(pack.FontNamed(name), decoders.FontNamed(name)) << name; });
  std::ranges::for_each(decoders.Ids(), [&](std::string const& id) {
    auto const entry{ decoders.Find(id) };
    if (!entry || entry->animation.empty()) return;
    EXPECT_EQ(pack.Frames(entry->animation), decoders.Frames(entry->animation)) << entry->animation;
  });
}

}

TEST(ClassicPack, HoldsEveryImageAsTheDecodersDrawIt) {
  auto const decoders{ CataloguePack{ Catalogue{ Embedded() } } };
  std::ranges::for_each(PacksUnderTest(), [&](std::filesystem::path const& path) {
    auto const pack{ OpenPack(path) };
    ASSERT_TRUE(pack) << path << ": " << Describe(pack.error());
    std::ranges::for_each(pack->Ids(), [&](std::string const& id) {
      auto const loaded{ pack->Load(id) };
      ASSERT_TRUE(loaded) << id << ": " << Describe(loaded.error());
      EXPECT_EQ(*loaded, decoders.Load(id).value()) << id;
    });
    if (WholePackGiven()) ExpectComplete(*pack, decoders);
  });
}

TEST(ClassicPack, HoldsTheGamesPalettes) {
  std::ranges::for_each(PacksUnderTest(), [](std::filesystem::path const& path) {
    auto const pack{ OpenPack(path) };
    ASSERT_TRUE(pack) << path;
    EXPECT_TRUE(std::ranges::all_of(ClassicPalettes(), [&](NamedPalette const& palette) { return pack->Palette(palette.name) == Colours(palette.levels); }));
  });
}

}
