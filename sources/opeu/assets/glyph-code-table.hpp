#pragma once
// The glyph fonts' code tables: tables/glyph-codes.json holds the code of each glyph of the ending's font (EMSG.16P)
// in KOEI's double-byte code, glyph n answering code n (docs/asset-pack.md, "Tables"). The text font's codes are its
// ids (pack.json's fonts); MSG.16P's glyphs have no known codes.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/pack-error.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::assets::detail::glyph_code_table {

inline constexpr auto GLYPH_CODE_TABLE{ std::string_view{ "tables/glyph-codes.json" } };

[[nodiscard]] auto GlyphCodeTableText(std::span<std::uint32_t const> ending) -> std::string;

// The ending font's codes.
[[nodiscard]] auto ReadGlyphCodeTable(Bytes bytes) -> Packed<std::vector<std::uint32_t>>;

}

namespace opeu::assets {
using detail::glyph_code_table::GLYPH_CODE_TABLE;
using detail::glyph_code_table::GlyphCodeTableText;
using detail::glyph_code_table::ReadGlyphCodeTable;
}
