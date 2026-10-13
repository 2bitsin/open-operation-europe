#pragma once
// An asset pack: images by stable id, ready for texture upload, with the manifest that describes them. The classic
// pack holds the shipped pictures as the decoders draw them (tools/assets/pack.py builds it); a remaster pack holds
// replacements for some ids and sits over the classic one in a FallbackPack. docs/asset-pack.md is the format.

#include "opeu/assets/image.hpp"
#include "opeu/assets/manifest.hpp"
#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/pack-files.hpp"
#include "opeu/assets/texture.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::pack {

// An asset as loaded: its manifest entry, its RGBA image, and its index map when the game recolours it.
struct LoadedAsset {
  PackedAsset          entry;
  Texture              texture;
  std::optional<Image> indexed;

  auto operator==(LoadedAsset const&) const -> bool = default;
};

// What the renderer asks of a pack, whichever packs stand behind it: images by id, palettes, the animations' frames,
// the glyph fonts and the scale.
class AssetPack {
public:
  virtual ~AssetPack() = default;

  [[nodiscard]] virtual auto Find(std::string_view id) const -> std::optional<PackedAsset>                       = 0;
  [[nodiscard]] virtual auto Load(std::string_view id) const -> Packed<LoadedAsset>                              = 0;
  [[nodiscard]] virtual auto Palette(std::string_view name) const -> std::optional<PaletteColours>               = 0;
  [[nodiscard]] virtual auto Ids() const -> std::vector<std::string>                                             = 0; // sorted
  [[nodiscard]] virtual auto Frames(std::string_view animation) const -> std::optional<std::vector<std::string>> = 0; // the frame ids in order
  [[nodiscard]] virtual auto FontNamed(std::string_view name) const -> std::optional<Font>                       = 0;
  [[nodiscard]] virtual auto FontNames() const -> std::vector<std::string>                                       = 0; // sorted
  // How many image pixels a game pixel is, each way: 1 for the classic pack, 4 for a remaster at 4x.
  [[nodiscard]] virtual auto Scale() const -> unsigned = 0;

protected:
  AssetPack()                                        = default;
  AssetPack(AssetPack const&)                        = default;
  AssetPack(AssetPack&&) noexcept                    = default;
  auto operator=(AssetPack const&) -> AssetPack&     = default;
  auto operator=(AssetPack&&) noexcept -> AssetPack& = default;
};

// One pack: its files and its manifest (pack.json among the files).
class Pack final : public AssetPack {
public:
  [[nodiscard]] static auto Open(std::unique_ptr<PackFiles const> files) -> Packed<Pack>;

  [[nodiscard]] auto Find(std::string_view id) const -> std::optional<PackedAsset> override;
  [[nodiscard]] auto Load(std::string_view id) const -> Packed<LoadedAsset> override;
  [[nodiscard]] auto Palette(std::string_view name) const -> std::optional<PaletteColours> override;
  [[nodiscard]] auto Ids() const -> std::vector<std::string> override;
  [[nodiscard]] auto Frames(std::string_view animation) const -> std::optional<std::vector<std::string>> override;
  [[nodiscard]] auto FontNamed(std::string_view name) const -> std::optional<Font> override;
  [[nodiscard]] auto FontNames() const -> std::vector<std::string> override;
  [[nodiscard]] auto Scale() const -> unsigned override;

private:
  Pack(std::unique_ptr<PackFiles const> files, Manifest manifest);

  [[nodiscard]] auto LoadTexture(PackedAsset const& entry) const -> Packed<Texture>;
  [[nodiscard]] auto LoadIndexed(PackedAsset const& entry, Texture const& texture) const -> Packed<std::optional<Image>>;

  std::unique_ptr<PackFiles const> _files;
  Manifest                         _manifest;
};

// The pack at `path`: a directory holding pack.json, or an .opeupack archive.
[[nodiscard]] auto OpenPack(std::filesystem::path const& path) -> Packed<Pack>;

}

namespace opeu::assets {
using detail::pack::AssetPack;
using detail::pack::LoadedAsset;
using detail::pack::OpenPack;
using detail::pack::Pack;
}
