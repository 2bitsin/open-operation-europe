#pragma once
// The game's screen on SDL_Renderer: services::Display (what the game draws with) as blits, fills, points and copies
// on the render module's frame. The images the game hands over as bytes become textures once (byte-textures.hpp): from
// the pack by id when the bytes are a shipped asset, else decoded by the assets module's decoders, and painted in the
// colours the screen shows. Nothing models the VGA planes; where the game still computes with the screen's colour
// numbers (an XOR, AND or OR mode, a highlight, a sprite composed over the screen, a rectangle saved into its memory)
// the call reads the rectangle back and turns its colours into numbers (game-frame.hpp). docs/rendering.md is the
// design and the list of what differs from the indexed Canvas.

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/planar.hpp"
#include "opeu/render/byte-textures.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/game-frame.hpp"
#include "opeu/render/pen.hpp"
#include "opeu/render/screen-palette.hpp"
#include "opeu/render/texture-cache.hpp"
#include "opeu/services/machine-display.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::render::detail::render_display {

// A colour or mode word of the game's library, as services/vgalib.hpp describes it per routine.
using Word = std::uint16_t;

inline constexpr auto FRAME_PIXELS{ 640uz * 480uz };

class RenderDisplay final : public services::Display {
public:
  // The game's screen on `display`'s frame, shown and black in the start colours. The images whose bytes `index`
  // recognises come from `pack`; both outlive the display.
  RenderDisplay(render::Display display, assets::AssetPack const& pack, assets::ContentIndex const& index);

  // ---- the screen
  auto EnterGraphics() -> void override;
  auto LeaveGraphics() -> void override;
  auto Clear() -> void override;
  auto ShowIdentityPalette() -> void override;
  auto LoadPalette(std::uint8_t first, std::span<std::uint8_t const> levels) -> void override;
  // ---- images
  auto DrawColumns(services::Area area, services::Image image) -> bool override;
  auto DrawThreeByteColumns(services::Area area, services::Image image) -> bool override;
  auto DrawBits(services::Area area, services::Image image) -> bool override;
  auto DrawBitsOnPaper(services::Area area, services::Image image) -> void override;
  auto DrawShiftedBits(services::Area area, services::Image image) -> bool override;
  auto DrawShiftedColumns(services::Area area, std::span<std::uint8_t const> image) -> void override;
  auto SaveColumns(services::Area area, std::span<std::uint8_t> into) -> void override;
  auto CopyColumns(services::Area from, services::Point to) -> void override;
  auto DecodePicture(services::Point at, PictureByteReader const& reader) -> bool override;
  auto DecodeMaskedPicture(services::Point at, PictureByteReader const& reader) -> bool override;
  // ---- shapes
  auto Box(services::Area area, Word colour) -> bool override;
  auto Highlight(services::Area area, Word colour) -> bool override;
  auto Line(services::Corners ends, Word colour) -> void override;
  auto Fill(services::Corners corners, Word colour) -> void override;
  // ---- battle cells
  auto SaveCell(services::Point at, services::Cell into) -> void override;
  auto SaveShiftedCell(services::Point at, services::Cell into) -> void override;
  auto RestoreCell(services::Point at, services::CellImage cell) -> void override;
  auto RestoreShiftedCell(services::Point at, services::CellImage cell) -> void override;
  auto CopyCellBlock(services::Point from, services::Point to) -> void override;
  // ---- cell buffers in memory
  auto OverlayCell(services::Cell cell, services::CellImage image) -> void override;
  auto MaskCell(services::Cell cell, services::CellMask mask) -> void override;
  auto DrawCellGlyph(services::Cell cell, services::CellGlyph glyph, services::Point at) -> void override;
  auto DrawColouredCellGlyph(services::Cell cell, services::CellGlyph glyph, services::ColouredPlace place) -> void override;
  // ---- sprites
  auto ComposeSprite(services::Area area, services::Sprite const& sprite) -> void override;
  auto ComposeShiftedSprite(services::Area area, services::Sprite const& sprite) -> void override;

  // ---- the platform's side
  // The frame to the renderer's output (black while the screen shows text).
  auto Present() -> void;
  // The frame as Present shows it, 640x480 RGBA.
  [[nodiscard]] auto Capture() -> assets::Texture;
  // The frame as palette entries and every entry's 8-bit levels: what the services' indexed present takes today.
  auto               RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> frame, std::span<std::uint8_t, PALETTE_ENTRIES * 3> palette) -> void;
  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics {
    return _textures.Statistics();
  }

private:
  enum class Mode : std::uint8_t {
    TEXT,
    GRAPHICS,
  };

  auto               Reset(Mode mode) -> void;
  auto               ChangeColours() -> void;
  [[nodiscard]] auto Paint(Keying keying) const -> render::Paint;
  [[nodiscard]] auto DrawColumnImage(services::Area area, services::Image image, std::size_t planes) -> bool;
  auto               CopyColumnImage(services::Area area, services::Image image, std::size_t planes) -> void;
  auto               CombineColumnImage(services::Area area, services::Image image, std::size_t planes) -> void;
  auto               DrawWithPen(GameImage const& bits, Rect destination) -> void;
  [[nodiscard]] auto Shape(GameImage const& bits) -> SharedBaked;
  auto               DrawPicture(services::Point at, PictureByteReader const& reader, assets::Layout stream) -> bool;
  auto               PlotLine(std::span<Point const> points, Word colour) -> void;
  auto               SaveCellAt(Point at, services::Cell into) -> void;
  auto               RestoreCellAt(Point at, services::CellImage cell) -> void;
  auto               Compose(Rect area, assets::Image const& keep, assets::Image const& image) -> assets::Image;

  ScreenPalette _palette;
  GameFrame     _frame;
  TextureCache  _pack_textures;
  ByteTextures  _textures;
  BitPen        _pen;
  Mode          _mode{ Mode::GRAPHICS };
  Mode          _mode_before{ Mode::TEXT }; // the mode EnterGraphics left
};

}

namespace opeu::render {
using detail::render_display::FRAME_PIXELS;
using detail::render_display::RenderDisplay;
}
