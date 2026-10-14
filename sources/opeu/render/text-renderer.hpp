#pragma once
// Text on the Display: a run of glyphs in a font, a pen colour, an optional paper colour behind the cells, and a
// position. The interface is the extension point for a true-type renderer (SDL_ttf through conan, the original
// metrics: fixed cells, pen colours); the pack's fonts are drawn by AtlasTextRenderer.

#include "opeu/render/display.hpp"
#include "opeu/render/font-atlas.hpp"
#include "opeu/render/geometry.hpp"
#include "opeu/render/render-error.hpp"

#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::render::detail::text_renderer {

// The classic pack's fonts (the manifest's `fonts`).
inline constexpr auto TEXT_FONT{ std::string_view{ "text-glyph" } };                     // 8x16, codes 0x20..0x7F: the game's text
inline constexpr auto DIGIT_FONT{ std::string_view{ "digit-glyph" } };                   // 6x6: 0..9 and the blank (10)
inline constexpr auto SIDEWAYS_DIGIT_FONT{ std::string_view{ "sideways-digit-glyph" } }; // 8x5: the vertical scale's digits
inline constexpr auto MARKER_FONT{ std::string_view{ "marker-glyph" } };                 // 8x8: the objective and the flashing cell
inline constexpr auto ENDING_FONT{ std::string_view{ "ending-glyph" } };                 // 16x14: the ending's Chinese text
inline constexpr auto GAME_FONT{ std::string_view{ "game-glyph" } };                     // 16x14: the Chinese edition's main font

struct TextRun {
  std::string             font;            // a font of the pack (TEXT_FONT, ...)
  std::vector<GlyphIndex> glyphs;          // in the font's code order
  Rgba                    colour{ WHITE }; // the pen
  Point                   at{};            // the first cell's top left corner
  std::optional<Rgba>     paper;           // the cells filled with it first: text drawn opaque

  auto operator==(TextRun const&) const -> bool = default;
};

// The glyph numbers of `text` in a font whose glyph 0 has the code `first_code` (FontMetrics::first_code).
[[nodiscard]] inline auto GlyphsOf(std::string_view const text, std::uint32_t const first_code) -> std::vector<GlyphIndex> {
  return text | std::views::transform([first_code](char const c) { return static_cast<GlyphIndex>(static_cast<std::uint8_t>(c) - first_code); }) | std::ranges::to<std::vector>();
}

class TextRenderer {
public:
  virtual ~TextRenderer() = default;

  virtual auto               Draw(Display& display, TextRun const& run) const -> Rendered<void> = 0;
  [[nodiscard]] virtual auto Measure(TextRun const& run) const -> Rendered<Size>                = 0;

protected:
  TextRenderer()                                           = default;
  TextRenderer(TextRenderer const&)                        = default;
  TextRenderer(TextRenderer&&) noexcept                    = default;
  auto operator=(TextRenderer const&) -> TextRenderer&     = default;
  auto operator=(TextRenderer&&) noexcept -> TextRenderer& = default;
};

}

namespace opeu::render {
using detail::text_renderer::DIGIT_FONT;
using detail::text_renderer::ENDING_FONT;
using detail::text_renderer::GAME_FONT;
using detail::text_renderer::GlyphsOf;
using detail::text_renderer::MARKER_FONT;
using detail::text_renderer::SIDEWAYS_DIGIT_FONT;
using detail::text_renderer::TEXT_FONT;
using detail::text_renderer::TextRenderer;
using detail::text_renderer::TextRun;
}
