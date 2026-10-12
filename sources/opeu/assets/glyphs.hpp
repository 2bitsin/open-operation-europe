#pragma once
// The two 16x14 one-bit glyph fonts of KOEI's Traditional Chinese edition: EMSG.16P (the ending's 218 glyphs, in
// the order of its text) and MSG.16P (1,080 glyphs of the Chinese main game, which this edition never opens).
// EMSG and MSG are byte-identical copies of them.

#include "opeu/assets/image.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace opeu::assets::detail::glyphs {

inline constexpr auto GLYPH_WIDTH{ std::size_t{ 16 } };
inline constexpr auto GLYPH_HEIGHT{ std::size_t{ 14 } };
inline constexpr auto GLYPH_BYTES{ GLYPH_WIDTH / 8 * GLYPH_HEIGHT };
inline constexpr auto ENDING_GLYPHS{ std::size_t{ 218 } };
inline constexpr auto GAME_GLYPHS{ std::size_t{ 1080 } };

// Every glyph of a font file; a set bit is ink (index 1).
[[nodiscard]] auto DecodeGlyphs(Bytes bytes) -> Decoded<std::vector<Image>>;

// The ending's text codes, glyph n answering code n: KOEI's own double-byte code (lead bytes 0x81..0xD8), neither
// Shift-JIS nor Big5.
[[nodiscard]] auto EndingGlyphCodes() noexcept -> std::span<std::uint16_t const, ENDING_GLYPHS>;

}

namespace opeu::assets {
using detail::glyphs::DecodeGlyphs;
using detail::glyphs::ENDING_GLYPHS;
using detail::glyphs::EndingGlyphCodes;
using detail::glyphs::GAME_GLYPHS;
using detail::glyphs::GLYPH_BYTES;
using detail::glyphs::GLYPH_HEIGHT;
using detail::glyphs::GLYPH_WIDTH;
}
