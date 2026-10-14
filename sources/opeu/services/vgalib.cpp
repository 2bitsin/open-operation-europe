// The game's drawing library: each routine forwards to the screen's canvas (canvas.hpp holds the drawing).
#include "vgalib.hpp"

#include "canvas.hpp"

#include <algorithm>
#include <ranges>
#include <span>
#include <tuple>

using opeu::services::Drawn;
using opeu::services::Screen;

namespace {

// The game's result word: 0 drawn, -1 not.
[[nodiscard]] auto Status(Drawn const drawn) noexcept -> std::int16_t {
  return drawn ? 0 : -1;
}

// The bytes an image of `rows` rows of `row_bytes` bytes occupies.
[[nodiscard]] auto Image(std::uint8_t const* const data, std::size_t const row_bytes, std::size_t const rows) noexcept -> std::span<std::uint8_t const> {
  return { data, row_bytes * rows };
}

}

auto koei_vga_identity_palette() -> void {
  std::ranges::for_each(std::views::iota(0, 16), [](int const colour) { Screen().MapColour(static_cast<std::uint8_t>(colour), static_cast<std::uint8_t>(colour)); });
}

auto opeu_set_palette(std::uint8_t first, std::uint8_t count, std::uint8_t const* rgb) -> std::int16_t {
  auto const      index{ first & 0x0F };
  auto const      entries{ count > 16 ? 16 - index : int{ count } };
  std::span const levels{ rgb, static_cast<std::size_t>(entries) * 3 };
  std::ranges::for_each(std::views::iota(0, entries), [index, levels](int const entry) {
    auto const level{ [&](int const part) { return static_cast<std::uint8_t>(levels[static_cast<std::size_t>((entry * 3) + part)] << 2); } };
    Screen().SetPaletteEntry(static_cast<std::uint8_t>(index + entry), level(1), level(2), level(0));
  });
  return 0;
}

auto koei_266c(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 4uz, h), 4));
}

auto opeu_bitmap_restore(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 4uz, h), 4));
}

auto koei_27c2(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 3uz, h), 3));
}

auto opeu_blit_far(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 3uz, h), 3));
}

auto opeu_draw_near(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawBits(x, y, w, h, mode, Image(data, w, h)));
}

auto opeu_draw_far(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawBits(x, y, w, h, mode, Image(data, w, h)));
}

auto opeu_code_0000_28c5(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> void {
  std::ignore = Screen().DrawBitsOnPaper(x, y, w, h, mode, Image(data, w, h));
}

auto opeu_vga_glyph(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawShiftedBits(x, y, w, h, mode, Image(data, ((w - 1uz) >> 3) + 1, h)));
}

auto opeu_gfx_blit(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t {
  return Status(Screen().DrawShiftedBits(x, y, w, h, mode, Image(data, ((w - 1uz) >> 3) + 1, h)));
}

auto opeu_vga_clear() -> std::int16_t {
  Screen().Clear();
  return 0;
}

auto opeu_ui_box(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t color) -> std::int16_t {
  return Status(Screen().Box(x, y, w, h, color));
}

auto opeu_ui_highlight(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t color) -> std::int16_t {
  return Status(Screen().Highlight(x, y, w, h, color));
}

auto Koei2ce4(std::uint16_t x, std::uint16_t y, std::uint16_t color) -> void {
  Screen().Plot(x, y, color);
}

auto opeu_ui_line(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t color) -> void {
  Screen().Line(x1, y1, x2, y2, color);
}

auto Koei2e49(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* data) -> void {
  Screen().DrawShiftedColumns(x, y, w, h, Image(data, (((w + 0xFFFFuz) & 0xFFFF) + 1) * 4, (h != 0u) ? h : 0x10000uz));
}

auto koei_2e57(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* data) -> void {
  Screen().DrawShiftedColumns(x, y, w, h, Image(data, (((w + 0xFFFFuz) & 0xFFFF) + 1) * 4, (h != 0u) ? h : 0x10000uz));
}

auto koei_2e72(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t* dest) -> void {
  Screen().SaveColumns(x, y, w, h, { dest, w * 4uz * h });
}

auto opeu_bitmap_save(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t* dest) -> void {
  Screen().SaveColumns(x, y, w, h, { dest, w * 4uz * h });
}

auto opeu_ui_fill(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t color) -> void {
  Screen().Fill(x1, y1, x2, y2, color);
}

auto koei_2fea() -> void {
  Screen().EnterGraphics();
}

auto opeu_text_mode() -> void {
  Screen().RestoreMode();
}

auto opeu_screen_copy(std::uint16_t x1, std::uint16_t y1, std::uint16_t w, std::uint16_t h, std::uint16_t x2, std::uint16_t y2) -> void {
  Screen().CopyColumns(x1, y1, w, h, x2, y2);
}

auto opeu_picture_decode_masked(std::uint16_t x, std::uint16_t y, PictureByteReader const& reader) -> std::int16_t {
  return Status(Screen().DecodePicture(x, y, reader, true));
}

auto opeu_picture_decode(std::uint16_t x, std::uint16_t y, PictureByteReader const& reader) -> std::int16_t {
  return Status(Screen().DecodePicture(x, y, reader, false));
}

auto vgalib_init() -> void {
  Screen().Start();
}

auto VgalibRender(std::span<std::uint8_t, 640uz * 480> const frame, std::span<std::uint8_t, 768> const palette) -> bool {
  auto&      screen{ Screen() };
  auto const changed{ screen.TakeChanged() };
  screen.Render(frame, palette);
  return changed;
}
