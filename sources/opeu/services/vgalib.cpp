// The game's drawing library: each routine forwards to the screen's canvas (canvas.hpp holds the drawing).
#include "vgalib.hpp"

#include "canvas.hpp"

#include <span>

using opeu::services::Drawn;
using opeu::services::Screen;

namespace {

// The game's result word: 0 drawn, -1 not.
[[nodiscard]] auto Status(Drawn drawn) noexcept -> int16_t {
  return drawn ? 0 : -1;
}

// The bytes an image of `rows` rows of `row_bytes` bytes occupies.
[[nodiscard]] auto Image(const uint8_t* data, std::size_t row_bytes, std::size_t rows) noexcept
    -> std::span<uint8_t const> {
  return { data, row_bytes * rows };
}

}

void koei_vga_identity_palette(void) {
  for (uint8_t i{}; i < 16; i++) Screen().MapColour(i, i);
}

int16_t opeu_set_palette(uint8_t first, uint8_t count, const uint8_t* rgb) {
  auto const index{ static_cast<uint8_t>(first & 0x0F) };
  auto const n{ count > 16 ? static_cast<uint8_t>(16 - index) : count };
  for (uint8_t i{}; i < n; i++, rgb += 3)
    Screen().SetPaletteEntry(static_cast<uint8_t>(index + i), static_cast<uint8_t>(rgb[1] << 2),
                             static_cast<uint8_t>(rgb[2] << 2), static_cast<uint8_t>(rgb[0] << 2));
  return 0;
}

int16_t koei_266c(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 4uz, h), 4));
}

int16_t opeu_bitmap_restore(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 4uz, h), 4));
}

int16_t koei_27c2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 3uz, h), 3));
}

int16_t opeu_blit_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawColumns(x, y, w, h, mode, Image(data, w * 3uz, h), 3));
}

int16_t opeu_draw_near(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawBits(x, y, w, h, mode, Image(data, w, h)));
}

int16_t opeu_draw_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawBits(x, y, w, h, mode, Image(data, w, h)));
}

void opeu_code_0000_28c5(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  (void)Screen().DrawBitsOnPaper(x, y, w, h, mode, Image(data, w, h));
}

int16_t opeu_vga_glyph(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawShiftedBits(x, y, w, h, mode, Image(data, ((w - 1uz) >> 3) + 1, h)));
}

int16_t opeu_gfx_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data) {
  return Status(Screen().DrawShiftedBits(x, y, w, h, mode, Image(data, ((w - 1uz) >> 3) + 1, h)));
}

int16_t opeu_vga_clear(void) {
  Screen().Clear();
  return 0;
}

int16_t opeu_ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  return Status(Screen().Box(x, y, w, h, color));
}

int16_t opeu_ui_highlight(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
  return Status(Screen().Highlight(x, y, w, h, color));
}

void koei_2ce4(uint16_t x, uint16_t y, uint16_t color) {
  Screen().Plot(x, y, color);
}

void opeu_ui_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  Screen().Line(x1, y1, x2, y2, color);
}

void koei_2e49(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  Screen().DrawShiftedColumns(x, y, w, h, Image(data, (static_cast<uint16_t>(w - 1) + 1uz) * 4, h ? h : 0x10000uz));
}

void koei_2e57(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data) {
  Screen().DrawShiftedColumns(x, y, w, h, Image(data, (static_cast<uint16_t>(w - 1) + 1uz) * 4, h ? h : 0x10000uz));
}

void koei_2e72(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest) {
  Screen().SaveColumns(x, y, w, h, { dest, w * 4uz * h });
}

void opeu_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest) {
  Screen().SaveColumns(x, y, w, h, { dest, w * 4uz * h });
}

void opeu_ui_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
  Screen().Fill(x1, y1, x2, y2, color);
}

void koei_2fea(void) {
  Screen().EnterGraphics();
}

void opeu_text_mode(void) {
  Screen().RestoreMode();
}

void opeu_screen_copy(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t x2, uint16_t y2) {
  Screen().CopyColumns(x1, y1, w, h, x2, y2);
}

int16_t opeu_picture_decode_masked(uint16_t x, uint16_t y, PictureByteReader reader) {
  return Status(Screen().DecodePicture(x, y, reader, true));
}

int16_t opeu_picture_decode(uint16_t x, uint16_t y, PictureByteReader reader) {
  return Status(Screen().DecodePicture(x, y, reader, false));
}

void vgalib_init(void) {
  Screen().Start();
}

int vgalib_render(uint8_t* out, uint8_t pal[768]) {
  auto&      screen{ Screen() };
  auto const changed{ screen.TakeChanged() };
  screen.Render(
      std::span<uint8_t, opeu::services::detail::canvas::PIXELS>{ out, opeu::services::detail::canvas::PIXELS },
      std::span<uint8_t, 768>{ pal, 768 });
  return changed;
}
