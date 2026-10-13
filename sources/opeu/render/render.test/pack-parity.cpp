// The renderer draws a built pack exactly as the decoders draw the shipped files: for every asset of the pack, as the
// pack holds it and in every palette the game shows it in, a TextureCache over the pack and one over the decoders'
// CataloguePack put the same pixels on screen. By default over the classic sample (render.test/packs/); the whole
// classic pack when OPEU_CLASSIC_PACK names it, which tools/assets/pack.test does on every test run.
#include "headless.hpp"

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/source.hpp"
#include "opeu/render/texture-cache.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace opeu::render::detail::pack_parity_test {

using test::OpenHeadless;

constexpr auto SHIPPED{ std::string_view{ "../opeu/assets.embed" } };

[[nodiscard]] auto PackUnderTest() -> std::filesystem::path {
  return std::filesystem::path{ test::Environment("OPEU_CLASSIC_PACK").value_or(std::string{ test::CLASSIC.path }) };
}

// The pack's own image (no palette), then every palette the game shows the asset in.
[[nodiscard]] auto KeysOf(assets::PackedAsset const& entry) -> std::vector<TextureKey> {
  auto keys{ std::vector{ TextureKey{ .asset = entry.id, .palette = {} } } };
  std::ranges::transform(entry.palettes, std::back_inserter(keys), [&entry](std::string const& palette) { return TextureKey{ .asset = entry.id, .palette = palette }; });
  return keys;
}

class Parity {
public:
  Parity(Display& display, assets::AssetPack const& pack, assets::AssetPack const& decoders)
      : _display{ display }, _built{ display.Renderer(), pack, 1 }, _decoded{ display.Renderer(), decoders, 1 } { }

  auto ExpectSame(TextureKey const& key) -> void {
    auto const built{ Shown(_built, key) };
    auto const decoded{ Shown(_decoded, key) };
    EXPECT_EQ(built, decoded) << key.asset << " in " << (key.palette.empty() ? "its own image" : key.palette);
  }

private:
  // The texture drawn alone on a transparent frame, read back at the game's size.
  [[nodiscard]] auto Shown(TextureCache& cache, TextureKey const& key) -> assets::Texture {
    auto const baked{ cache.Fetch(key) };
    if (!baked) {
      ADD_FAILURE() << key.asset << ": " << baked.error().detail;
      return assets::Texture{};
    }
    _display.get().Clear(Rgba{ .a = 0 });
    _display.get().Draw((*baked)->texture, Blit{ .destination = At(Point{}, (*baked)->size) });
    return _display.get().Read(At(Point{}, (*baked)->size)).value();
  }

  std::reference_wrapper<Display> _display;
  TextureCache                    _built;
  TextureCache                    _decoded;
};

TEST(PackParity, BakesEveryTextureOfThePackAsFromTheDecoders) {
  auto const shipped{ assets::DirectorySource::Open(SHIPPED).value() };
  auto const decoders{ assets::CataloguePack{ assets::Catalogue{ shipped } } };
  auto const pack{ assets::OpenPack(PackUnderTest()) };
  ASSERT_TRUE(pack) << PackUnderTest() << ": " << assets::Describe(pack.error());
  auto headless{ OpenHeadless() };
  headless.display.Begin();
  auto parity{
    Parity{ headless.display, *pack, decoders }
  };
  std::ranges::for_each(pack->Ids(), [&](std::string const& id) {
    auto const entry{ pack->Find(id) };
    ASSERT_TRUE(entry) << id;
    std::ranges::for_each(KeysOf(*entry), [&parity](TextureKey const& key) { parity.ExpectSame(key); });
  });
}

}
