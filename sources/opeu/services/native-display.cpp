// The native display: each method forwards to its vgalib.hpp or syslib-gfx.hpp function. The handles keep the
// screen's bytes here, in the formats the game kept them in its own memory; an overlay is the game's XOR, drawn twice.
#include "native-machine.hpp"

#include "hostinput.hpp"
#include "platform.hpp"

#include <algorithm>
#include <ranges>
#include <span>
#include <type_traits>
#include <variant>

namespace opeu::services::detail::native_machine {
namespace {

// The drawing library's 16-bit argument word (a coordinate, a count, a colour or pen word).
using Word = decltype(Area::x);

// The drawing library's result word: 0 drawn, -1 not.
[[nodiscard]] auto Drawn(std::int16_t const status) noexcept -> bool {
  return status == 0;
}

// A loop count of 0 runs 65536 times, as the library's.
[[nodiscard]] auto Lines(Word const count) noexcept -> std::size_t {
  return count != 0 ? count : 0x10000uz;
}

// The scratch a sprite is composed in: four colour-bit blocks of the rectangle (the library's sizes).
[[nodiscard]] auto SpriteScratch(Area const area) -> std::vector<std::uint8_t> {
  auto const block{ (std::size_t{ area.width } * area.height) & 0xFFFF };
  auto const count{ block != 0 ? block : 0x10000uz };
  return std::vector<std::uint8_t>(std::max(4 * area.width * Lines(area.height), 3 * block + count));
}

[[nodiscard]] auto ShiftedSpriteScratch(Area const area) -> std::vector<std::uint8_t> {
  auto const columns{ (std::size_t{ area.width } >> 3) + 1 };
  auto const width{ ((columns + 0xFFFE) & 0xFFFF) + 2 };
  return std::vector<std::uint8_t>(4 * width * Lines(area.height));
}

// The overlay's XOR: the library's pen 1 with the exchanged bits.
constexpr Word XOR_PEN{ 0x0100 };
constexpr Word XOR_LINE{ 0x0010 };
// A stamp's cut and ink: the library's AND pen with every colour bit, and its OR pen.
constexpr Word AND_ALL{ 0x020F };
constexpr Word OR_PEN{ 0x0300 };

}

// ---- handles ------------------------------------------------------------------------------------------------------

auto NativeDisplay::Save(Area const area) -> SnapshotId {
  auto bytes{ std::vector<std::uint8_t>(4uz * area.width * area.height) };
  opeu_bitmap_save(area.x, area.y, area.width, area.height, bytes.data());
  return _saved.Keep(Saved{ .area = area, .bytes = std::move(bytes) });
}

auto NativeDisplay::Restore(SnapshotId const snapshot, Point const at) -> void {
  auto const saved{ _saved.Find(snapshot) };
  if (!saved) return;
  auto const& [area, bytes]{ saved->get() };
  std::ignore = opeu_bitmap_restore(at.x, at.y, area.width, area.height, 0, bytes.data());
}

auto NativeDisplay::TakeCell(Point const at) -> CellId {
  auto cell{ CellBytes{} };
  opeu_vram_save(at.x, at.y, cell.data());
  return _cells.Keep(cell);
}

auto NativeDisplay::TakeShiftedCell(Point const at) -> CellId {
  auto cell{ CellBytes{} };
  opeu_vram_save_odd(at.x, at.y, cell.data());
  return _cells.Keep(cell);
}

auto NativeDisplay::MaskInCell(CellId const cell, CellMask const mask) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_load(kept->get().data(), mask.data());
}

auto NativeDisplay::OverlayInCell(CellId const cell, CellImage const image) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_draw(kept->get().data(), image.data());
}

auto NativeDisplay::GlyphInCell(CellId const cell, CellGlyph const glyph, Point const at) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_glyph(kept->get().data(), glyph.data(), at.x, at.y);
}

auto NativeDisplay::ColouredGlyphInCell(CellId const cell, CellGlyph const glyph, ColouredPlace const place) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_glyph_color(kept->get().data(), glyph.data(), place.at.x, place.at.y, place.colour);
}

auto NativeDisplay::PutCell(Point const at, CellId const cell) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_restore(at.x, at.y, kept->get().data());
}

auto NativeDisplay::PutShiftedCell(Point const at, CellId const cell) -> void {
  if (auto const kept{ _cells.Find(cell) }) opeu_vram_restore_odd(at.x, at.y, kept->get().data());
}

auto NativeDisplay::DrawSprite(Area const area, SpriteImage const& sprite) -> void {
  auto scratch{ SpriteScratch(area) };
  ComposeSprite(area, { .and_mask = sprite.and_mask, .or_image = sprite.or_image, .buffer = scratch, .flags = sprite.flags });
}

auto NativeDisplay::DrawShiftedSprite(Area const area, SpriteImage const& sprite) -> void {
  auto scratch{ ShiftedSpriteScratch(area) };
  ComposeShiftedSprite(area, { .and_mask = sprite.and_mask, .or_image = sprite.or_image, .buffer = scratch, .flags = sprite.flags });
}

// The library's AND pen with every colour bit clears under the cut's 0 bits; its OR pen sets the ink's colour there.
auto NativeDisplay::DrawStamp(Area const area, Stamp const& stamp) -> void {
  std::ignore = opeu_draw_far(area.x, area.y, area.width, area.height, AND_ALL, stamp.cut.data());
  if (!stamp.ink.empty()) std::ignore = opeu_draw_far(area.x, area.y, area.width, area.height, static_cast<Word>(OR_PEN | (stamp.colour & 0x0F)), stamp.ink.data());
}

auto NativeDisplay::DrawShiftedStamp(Area const area, Stamp const& stamp) -> void {
  std::ignore = opeu_gfx_blit(area.x, area.y, area.width, area.height, AND_ALL, stamp.cut.data());
  if (!stamp.ink.empty()) std::ignore = opeu_gfx_blit(area.x, area.y, area.width, area.height, static_cast<Word>(OR_PEN | (stamp.colour & 0x0F)), stamp.ink.data());
}

auto NativeDisplay::ShowOverlay(Overlay const& overlay) -> OverlayId {
  auto shown{
    Shown{ .overlay = overlay, .bits = {} }
  };
  if (std::holds_alternative<Mark>(overlay.shape)) { // the bits the game lent are kept to draw it off; moving keeps them
    auto const& mark{ std::get<Mark>(overlay.shape) };
    shown.bits          = mark.bits | std::ranges::to<std::vector>();
    shown.overlay.shape = Mark{ .pixels = mark.pixels, .bits = shown.bits };
  }
  Toggle(shown);
  return _overlays.Keep(std::move(shown));
}

auto NativeDisplay::HideOverlay(OverlayId const overlay) -> void {
  if (auto const shown{ _overlays.Release(overlay) }) Toggle(*shown);
}

// The overlay's XOR, which a second time puts the pixels back.
auto NativeDisplay::Toggle(Shown const& shown) -> void {
  auto const exchange{ static_cast<Word>(shown.overlay.exchange & 0x0F) };
  std::visit(
      [exchange](auto const& shape) {
        using Shape = std::remove_cvref_t<decltype(shape)>;
        if constexpr (std::is_same_v<Shape, Bar>) {
          std::ignore = opeu_ui_highlight(shape.columns.x, shape.columns.y, shape.columns.width, shape.columns.height, XOR_PEN | exchange);
        } else if constexpr (std::is_same_v<Shape, Mark>) {
          std::ignore = opeu_gfx_blit(shape.pixels.x, shape.pixels.y, shape.pixels.width, shape.pixels.height, XOR_PEN | exchange, shape.bits.data());
        } else {
          opeu_ui_line(shape.ends.from.x, shape.ends.from.y, shape.ends.to.x, shape.ends.to.y, static_cast<Word>(shape.dashes << 8 | XOR_LINE | exchange));
        }
      },
      shown.overlay.shape);
}

// The frame when anything on it changed (or the pointer moved), with the pointer over it.
auto NativeDisplay::Present() -> void {
  auto const frame{ std::span{ _frame }.first<640uz * 480uz>() };
  auto const changed{ VgalibRender(frame, _palette) };
  if (_presented && !changed && !PointerDirty()) return;
  DrawPointer(frame, _palette);
  platform::Present({ .pixels = _frame, .width = 640, .height = 480, .palette = _palette });
  _presented = true;
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
