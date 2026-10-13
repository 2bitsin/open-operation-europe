#pragma once
// Every picture, sprite, tile, portrait, silhouette, mask and glyph in the shipped files and in the program's own
// glyph tables (program-glyphs.hpp), with a stable id and where and how it is stored. None of the files has a header
// or a directory: the places are the game's own.

#include "opeu/assets/image.hpp"
#include "opeu/assets/source.hpp"
#include "opeu/assets/texture.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::catalogue {

enum class Layout : std::uint8_t {
  PLANES_4,      // interleaved planes, four per byte column
  PLANES_3,      // interleaved planes, three per byte column (colours 0..7)
  BITMAP,        // one bit per pixel, drawn in the asset's ink
  RUNS_BITMAP,   // a BITMAP packed with the 00/FF run-length code
  CELL,          // the 650-byte battle sprite with its mask
  MASKED_SPRITE, // interleaved four-plane image plus a separate AND mask
  STREAM_8,      // KOEI picture stream, eight colours
  STREAM_16,     // KOEI picture stream, sixteen colours
};

enum class Role : std::uint8_t {
  PICTURE,
  TILE,
  SPRITE,
  PORTRAIT,
  SILHOUETTE,
  MASK,
  CAPTION,
  GLYPH
};

struct Asset {
  std::string      id;        // "face/000", "hexunit/icon-03/facing-1", ...
  std::string_view file;      // the shipped file name, or a program table's (PROGRAM_TEXT_FONT, ...)
  std::string_view group;     // the manifest it belongs to
  std::uint32_t    at{};      // byte offset of the record in the file
  std::uint32_t    mask_at{}; // MASKED_SPRITE: the AND mask's offset
  Layout           layout{};
  unsigned         width_bytes{}; // raw layouts: bytes per row (a stream states its own size)
  std::uint32_t    width{};       // BITMAP: pixels per row when fewer than width_bytes x 8 (the left ones), else 0
  unsigned         height{};      // raw layouts: rows
  Role             role{};
  std::string_view palette;           // the palette the game shows it in (see palette.hpp)
  std::uint8_t     ink{};             // BITMAP, RUNS_BITMAP: the colour of the shape
  bool             ink_where_clear{}; // BITMAP, RUNS_BITMAP: the shape is the clear bits (an AND mask)
  bool             side_coloured{};   // a unit icon: planes 0 and 1 swap for side 0
  bool             hour_coloured{};   // colours 8..15 follow the hour (dawn, day, dusk, night)
  std::string      animation;         // the sequence a frame belongs to, or empty
  unsigned         frame{};
  std::string      use; // where the game draws it; empty when nothing does
};

// Decodes one asset from the bytes of its file.
[[nodiscard]] auto DecodeAsset(Bytes file, Asset const& asset) -> Decoded<Image>;

// A unit icon as side 0 shows it: planes 0 and 1 exchanged (index bits 0 and 1 swapped).
[[nodiscard]] auto SideZeroColours(Image const& image) -> Image;

[[nodiscard]] auto Spelling(Layout layout) noexcept -> std::string_view;
[[nodiscard]] auto Spelling(Role role) noexcept -> std::string_view;

// Every graphic asset of the 26 shipped files, in a fixed order, decoded on demand from the files of a source.
class Catalogue {
public:
  explicit Catalogue(Source const& source);

  [[nodiscard]] auto Assets() const noexcept -> std::span<Asset const> {
    return _assets;
  }
  [[nodiscard]] auto Find(std::string_view id) const -> std::optional<Asset>;
  // The bytes of a shipped file or a program table.
  [[nodiscard]] auto File(std::string_view name) const -> Decoded<Bytes>;
  [[nodiscard]] auto Decode(Asset const& asset) const -> Decoded<Image>;
  // The asset as the screen shows it in its palette (for a unit icon, side 1's colours; SideZeroColours on the
  // decoded image gives side 0's).
  [[nodiscard]] auto DecodeTexture(Asset const& asset) const -> Decoded<Texture>;

private:
  std::reference_wrapper<Source const> _source;
  std::vector<Asset>                   _assets;
};

// ENDGRP.NPK ends in the ending program's nine-byte one-shot token (blank in the shipped file).
inline constexpr auto EPILOGUE_TOKEN_AT{ std::uint32_t{ 0x3DA09 } };
inline constexpr auto EPILOGUE_TOKEN_BYTES{ std::uint32_t{ 9 } };

}

namespace opeu::assets {
using detail::catalogue::Asset;
using detail::catalogue::Catalogue;
using detail::catalogue::DecodeAsset;
using detail::catalogue::EPILOGUE_TOKEN_AT;
using detail::catalogue::EPILOGUE_TOKEN_BYTES;
using detail::catalogue::Layout;
using detail::catalogue::Role;
using detail::catalogue::SideZeroColours;
using detail::catalogue::Spelling;
}
