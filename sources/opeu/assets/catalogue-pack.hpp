#pragma once
// The classic pack drawn by the decoders, straight from the shipped files: the entries, images, index maps,
// palettes, animations and fonts tools/assets/pack.py writes, without the PNGs in between. The renderer reads a built
// pack; this one is the reference a built pack is proven against (assets.test/classic-pack.cpp,
// render.test/pack-parity.cpp) and a pack for a tool that has the shipped files at hand.

#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/pack.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::catalogue_pack {

inline constexpr auto SIDE_ZERO{ std::string_view{ "side-0" } };

class CataloguePack final : public AssetPack {
public:
  // The catalogue's source must outlive the pack.
  explicit CataloguePack(Catalogue catalogue);

  [[nodiscard]] auto Find(std::string_view id) const -> std::optional<PackedAsset> override;
  [[nodiscard]] auto Load(std::string_view id) const -> Packed<LoadedAsset> override;
  [[nodiscard]] auto Palette(std::string_view name) const -> std::optional<PaletteColours> override;
  [[nodiscard]] auto Ids() const -> std::vector<std::string> override;
  [[nodiscard]] auto Frames(std::string_view animation) const -> std::optional<std::vector<std::string>> override;
  [[nodiscard]] auto FontNamed(std::string_view name) const -> std::optional<Font> override;
  [[nodiscard]] auto FontNames() const -> std::vector<std::string> override;
  [[nodiscard]] auto Scale() const -> std::uint32_t override;

private:
  // A pack image: a catalogue asset, or its side-0 variant.
  struct Listed {
    Asset       asset;
    std::string variant; // empty, or SIDE_ZERO
  };
  struct Drawn {
    PackedAsset entry;
    Image       image; // in the asset's colours (side 0's for the variant)
  };

  [[nodiscard]] auto Draw(std::string_view id) const -> Packed<Drawn>;

  Catalogue                                                    _catalogue;
  std::map<std::string, Listed, std::less<>>                   _listed;
  std::map<std::string, std::vector<std::string>, std::less<>> _animations;
  std::map<std::string, Font, std::less<>>                     _fonts;
};

}

namespace opeu::assets {
using detail::catalogue_pack::CataloguePack;
using detail::catalogue_pack::SIDE_ZERO;
}
