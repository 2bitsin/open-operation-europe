#pragma once
// A pack over another: every id the preferred pack holds comes from it, every other id from the fallback; palettes,
// animations and fonts the same by name. A remaster pack over the classic pack is one: the game always runs on a whole
// pack, however few assets are remastered.

#include "opeu/assets/pack.hpp"

#include <memory>

namespace opeu::assets::detail::fallback_pack {

class FallbackPack final : public AssetPack {
public:
  FallbackPack(std::unique_ptr<AssetPack const> preferred, std::unique_ptr<AssetPack const> fallback);

  [[nodiscard]] auto Find(std::string_view id) const -> std::optional<PackedAsset> override;
  [[nodiscard]] auto Load(std::string_view id) const -> Packed<LoadedAsset> override;
  [[nodiscard]] auto Palette(std::string_view name) const -> std::optional<PaletteColours> override;
  [[nodiscard]] auto Ids() const -> std::vector<std::string> override;
  [[nodiscard]] auto Frames(std::string_view animation) const -> std::optional<std::vector<std::string>> override;
  [[nodiscard]] auto FontNamed(std::string_view name) const -> std::optional<Font> override;
  [[nodiscard]] auto FontNames() const -> std::vector<std::string> override;
  // The preferred pack's: a replaced image is drawn at its own scale, the others at the fallback's.
  [[nodiscard]] auto Scale() const -> unsigned override;

private:
  // The pack that answers for `id`.
  [[nodiscard]] auto Holder(std::string_view id) const -> AssetPack const&;

  std::unique_ptr<AssetPack const> _preferred;
  std::unique_ptr<AssetPack const> _fallback;
};

}

namespace opeu::assets {
using detail::fallback_pack::FallbackPack;
}
