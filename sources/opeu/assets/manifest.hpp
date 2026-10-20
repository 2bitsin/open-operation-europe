#pragma once
// pack.json, the manifest of an asset pack: every asset's stable id and how to draw it, the palettes, the animations'
// frame sequences and the glyph fonts. docs/asset-pack.md is the schema.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/pack-error.hpp"
#include "opeu/assets/palette.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::manifest {

using namespace std::string_literals;
using namespace std::string_view_literals;

inline constexpr auto PACK_FORMAT{ "opeu-pack/1"sv };
inline constexpr auto MANIFEST_PATH{ "pack.json"sv };

using PaletteColours = std::array<Colour, 16>;

// One image of the pack. Geometry is the game's (the classic pixels); an image file holds width x height times the
// entry's scale (the pack's, unless the entry states its own).
struct PackedAsset {
  std::string                id;          // "portrait/118", "battle-unit/class-03/facing-1/side-0"
  std::string                asset_class; // the class it is grouped and remastered by ("portraits", "tiles", ...)
  std::string                role;        // picture, tile, sprite, portrait, silhouette, mask, caption, glyph
  std::size_t                width{};
  std::size_t                height{};
  std::uint32_t              scale{ 1 }; // image pixels per game pixel, each way
  std::string                palette;    // the palette the image is drawn in
  std::vector<std::string>   palettes;   // every palette the game shows it in
  bool                       mask{};     // some pixels let the background show
  std::vector<std::uint8_t>  colours;    // the palette indices it draws
  std::string                image;      // RGBA PNG, relative to the pack
  std::optional<std::string> recolour;   // why the game shows it in other colours: hour, ink or scene
  std::optional<std::string> indexed;    // the index map (8-bit grey, 255 transparent) of a recoloured asset
  std::string                animation;  // the sequence it is a frame of, or empty
  unsigned                   frame{};
  std::string                variant_of;  // the asset this image is a variant of, or empty
  std::string                variant;     // "side-0", or empty
  std::string                use;         // where the game draws it
  std::string                description; // what the picture shows, in a line

  auto operator==(PackedAsset const&) const -> bool = default;
};

struct Font {
  std::size_t              cell_width{};
  std::size_t              cell_height{};
  std::uint32_t            first_code{}; // the code of the first glyph: 0x20 for the text font, 0 where a glyph's number is its code
  std::vector<std::string> glyphs;       // ids in code order

  auto operator==(Font const&) const -> bool = default;
};

class Manifest {
public:
  using Assets     = std::map<std::string, PackedAsset, std::less<>>;
  using Palettes   = std::map<std::string, PaletteColours, std::less<>>;
  using Animations = std::map<std::string, std::vector<std::string>, std::less<>>;
  using Fonts      = std::map<std::string, Font, std::less<>>;

  struct Contents {
    std::string   name;
    std::uint32_t scale{ 1 };
    Assets        assets;
    Palettes      palettes;
    Animations    animations;
    Fonts         fonts;
  };

  explicit Manifest(Contents contents);

  [[nodiscard]] static auto Parse(Bytes json) -> Packed<Manifest>;

  [[nodiscard]] auto Name() const noexcept -> std::string_view {
    return _contents.name;
  }
  // How many image pixels a game pixel is, each way (1 for the classic pack, 4 for a remaster at 4x).
  [[nodiscard]] auto Scale() const noexcept -> std::uint32_t {
    return _contents.scale;
  }
  [[nodiscard]] auto Find(std::string_view id) const -> std::optional<PackedAsset>;
  [[nodiscard]] auto Ids() const -> std::vector<std::string>; // sorted
  [[nodiscard]] auto Palette(std::string_view name) const -> std::optional<PaletteColours>;
  [[nodiscard]] auto Frames(std::string_view animation) const -> std::optional<std::vector<std::string>>;
  [[nodiscard]] auto FontNamed(std::string_view name) const -> std::optional<Font>;
  [[nodiscard]] auto FontNames() const -> std::vector<std::string>; // sorted

private:
  Contents _contents;
};

}

namespace opeu::assets {
using detail::manifest::Font;
using detail::manifest::Manifest;
using detail::manifest::MANIFEST_PATH;
using detail::manifest::PACK_FORMAT;
using detail::manifest::PackedAsset;
using detail::manifest::PaletteColours;
}
