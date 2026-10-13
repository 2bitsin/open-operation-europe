// The native display: each method forwards to its vgalib.hpp or syslib-gfx.hpp function.
#include "native-machine.hpp"

namespace opeu::services::detail::native_machine {
namespace {

// The drawing library's result word: 0 drawn, -1 not.
[[nodiscard]] auto Drawn(std::int16_t const status) noexcept -> bool {
  return status == 0;
}

}

auto NativeDisplay::EnterGraphics() -> void {
  koei_2fea();
}

auto NativeDisplay::LeaveGraphics() -> void {
  opeu_text_mode();
}

auto NativeDisplay::Clear() -> void {
  opeu_vga_clear();
}

auto NativeDisplay::ShowIdentityPalette() -> void {
  koei_vga_identity_palette();
}

auto NativeDisplay::LoadPalette(std::uint8_t const first, std::span<std::uint8_t const> const levels) -> void {
  opeu_set_palette(first, static_cast<std::uint8_t>(levels.size() / 3), levels.data());
}

auto NativeDisplay::DrawColumns(Area const area, Image const image) -> bool {
  return Drawn(opeu_bitmap_restore(area.x, area.y, area.width, area.height, image.mode, image.bytes.data()));
}

auto NativeDisplay::DrawThreeByteColumns(Area const area, Image const image) -> bool {
  return Drawn(opeu_blit_far(area.x, area.y, area.width, area.height, image.mode, image.bytes.data()));
}

auto NativeDisplay::DrawBits(Area const area, Image const image) -> bool {
  return Drawn(opeu_draw_far(area.x, area.y, area.width, area.height, image.mode, image.bytes.data()));
}

auto NativeDisplay::DrawBitsOnPaper(Area const area, Image const image) -> void {
  opeu_code_0000_28c5(area.x, area.y, area.width, area.height, image.mode, image.bytes.data());
}

auto NativeDisplay::DrawShiftedBits(Area const area, Image const image) -> bool {
  return Drawn(opeu_gfx_blit(area.x, area.y, area.width, area.height, image.mode, image.bytes.data()));
}

auto NativeDisplay::DrawShiftedColumns(Area const area, std::span<std::uint8_t const> const image) -> void {
  koei_2e57(area.x, area.y, area.width, area.height, image.data());
}

auto NativeDisplay::SaveColumns(Area const area, std::span<std::uint8_t> const into) -> void {
  opeu_bitmap_save(area.x, area.y, area.width, area.height, into.data());
}

auto NativeDisplay::CopyColumns(Area const from, Point const to) -> void {
  opeu_screen_copy(from.x, from.y, from.width, from.height, to.x, to.y);
}

auto NativeDisplay::DecodePicture(Point const at, PictureByteReader const& reader) -> bool {
  return Drawn(opeu_picture_decode(at.x, at.y, reader));
}

auto NativeDisplay::DecodeMaskedPicture(Point const at, PictureByteReader const& reader) -> bool {
  return Drawn(opeu_picture_decode_masked(at.x, at.y, reader));
}

auto NativeDisplay::Box(Area const area, std::uint16_t const colour) -> bool {
  return Drawn(opeu_ui_box(area.x, area.y, area.width, area.height, colour));
}

auto NativeDisplay::Highlight(Area const area, std::uint16_t const colour) -> bool {
  return Drawn(opeu_ui_highlight(area.x, area.y, area.width, area.height, colour));
}

auto NativeDisplay::Line(Corners const ends, std::uint16_t const colour) -> void {
  opeu_ui_line(ends.from.x, ends.from.y, ends.to.x, ends.to.y, colour);
}

auto NativeDisplay::Fill(Corners const corners, std::uint16_t const colour) -> void {
  opeu_ui_fill(corners.from.x, corners.from.y, corners.to.x, corners.to.y, colour);
}

auto NativeDisplay::SaveCell(Point const at, Cell const into) -> void {
  opeu_vram_save(at.x, at.y, into.data());
}

auto NativeDisplay::SaveShiftedCell(Point const at, Cell const into) -> void {
  opeu_vram_save_odd(at.x, at.y, into.data());
}

auto NativeDisplay::RestoreCell(Point const at, CellImage const cell) -> void {
  opeu_vram_restore(at.x, at.y, cell.data());
}

auto NativeDisplay::RestoreShiftedCell(Point const at, CellImage const cell) -> void {
  opeu_vram_restore_odd(at.x, at.y, cell.data());
}

auto NativeDisplay::CopyCellBlock(Point const from, Point const to) -> void {
  koei_3bc5(from.x, from.y, to.x, to.y);
}

auto NativeDisplay::OverlayCell(Cell const cell, CellImage const image) -> void {
  opeu_vram_draw(cell.data(), image.data());
}

auto NativeDisplay::MaskCell(Cell const cell, CellMask const mask) -> void {
  opeu_vram_load(cell.data(), mask.data());
}

auto NativeDisplay::DrawCellGlyph(Cell const cell, CellGlyph const glyph, Point const at) -> void {
  opeu_vram_glyph(cell.data(), glyph.data(), at.x, at.y);
}

auto NativeDisplay::DrawColouredCellGlyph(Cell const cell, CellGlyph const glyph, ColouredPlace const place) -> void {
  opeu_vram_glyph_color(cell.data(), glyph.data(), place.at.x, place.at.y, place.colour);
}

auto NativeDisplay::ComposeSprite(Area const area, Sprite const& sprite) -> void {
  opeu_sprite_compose(area.x, area.y, area.width, area.height, sprite.and_mask.data(), sprite.or_image.data(), sprite.buffer.data(), sprite.flags);
}

auto NativeDisplay::ComposeShiftedSprite(Area const area, Sprite const& sprite) -> void {
  koei_3f26(area.x, area.y, area.width, area.height, sprite.and_mask.data(), sprite.or_image.data(), sprite.buffer.data(), sprite.flags);
}

}
