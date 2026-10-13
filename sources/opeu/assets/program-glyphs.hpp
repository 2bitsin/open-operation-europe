#pragma once
// The glyphs the game's program carries in its own data rather than in a shipped file: the 8x16 text font of the
// codes 0x20..0x7F, the 6x6 digits of the map's scales and the battle panels, the same digits on their side (8x5) for
// the vertical scale, and the overview's two 8x8 markers. The catalogue reads them as files named PROGRAM_* through
// ProgramFile, in the one-bit layout of the shipped bitmaps (one byte a row, bit 7 the leftmost pixel).

#include "opeu/assets/bytes.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace opeu::assets::detail::program_glyphs {

inline constexpr auto PROGRAM_TEXT_FONT{ std::string_view{ "program/text-font" } };
inline constexpr auto PROGRAM_DIGITS{ std::string_view{ "program/digits" } };
inline constexpr auto PROGRAM_SIDEWAYS_DIGITS{ std::string_view{ "program/sideways-digits" } };
inline constexpr auto PROGRAM_MARKERS{ std::string_view{ "program/markers" } };

inline constexpr auto FIRST_TEXT_CODE{ std::size_t{ 0x20 } };
inline constexpr auto TEXT_GLYPHS{ std::size_t{ 96 } }; // codes 0x20..0x7F
inline constexpr auto TEXT_GLYPH_HEIGHT{ std::size_t{ 16 } };
inline constexpr auto DIGIT_GLYPHS{ std::size_t{ 11 } }; // 0..9, then the blank the game clears a digit's cell with
inline constexpr auto DIGIT_SIZE{ std::size_t{ 6 } };
inline constexpr auto SIDEWAYS_DIGIT_HEIGHT{ std::size_t{ 5 } };
inline constexpr auto MARKERS{ std::size_t{ 2 } }; // the objective, the flashing cell
inline constexpr auto MARKER_SIZE{ std::size_t{ 8 } };

// The bytes of a program table by its PROGRAM_* name; nothing for any other name.
[[nodiscard]] auto ProgramFile(std::string_view name) -> std::optional<Bytes>;

}

namespace opeu::assets {
using detail::program_glyphs::DIGIT_GLYPHS;
using detail::program_glyphs::DIGIT_SIZE;
using detail::program_glyphs::FIRST_TEXT_CODE;
using detail::program_glyphs::MARKER_SIZE;
using detail::program_glyphs::MARKERS;
using detail::program_glyphs::PROGRAM_DIGITS;
using detail::program_glyphs::PROGRAM_MARKERS;
using detail::program_glyphs::PROGRAM_SIDEWAYS_DIGITS;
using detail::program_glyphs::PROGRAM_TEXT_FONT;
using detail::program_glyphs::ProgramFile;
using detail::program_glyphs::SIDEWAYS_DIGIT_HEIGHT;
using detail::program_glyphs::TEXT_GLYPH_HEIGHT;
using detail::program_glyphs::TEXT_GLYPHS;
}
