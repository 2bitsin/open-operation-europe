#pragma once
// The game's screen on SDL_Renderer: services::Display (what the game draws with) as steps on the render module's
// frame (game-frame.hpp), each painted with blits, fills and points in the colours the screen shows. The images the
// game hands over as bytes become textures once (byte-textures.hpp): from the pack by id when the bytes are a shipped
// asset, else decoded by the assets module's decoders. Nothing models the VGA planes and nothing reads the frame back
// while the game draws: an XOR, AND or OR with the screen is what is under it drawn again through a colour lookup, a
// saved rectangle, a battle cell and a copy are what was drawn there, drawn again. Where the game still passes the
// screen through its own memory (SaveColumns, SaveCell, the cell buffers), the bytes it gets are a handle's mark, which
// it only ever hands back (docs/rendering.md has the proof per call site and the design).

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/frame-steps.hpp"
#include "opeu/render/game-frame.hpp"
#include "opeu/render/line-points.hpp"
#include "opeu/render/pen.hpp"
#include "opeu/render/screen-palette.hpp"
#include "opeu/services/kept-handles.hpp"
#include "opeu/services/machine-display.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace opeu::render::detail::render_display {

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
  // ---- battle cells through the game's memory: the bytes are a cell's handle
  auto SaveCell(services::Point at, services::Cell into) -> void override;
  auto SaveShiftedCell(services::Point at, services::Cell into) -> void override;
  auto RestoreCell(services::Point at, services::CellImage cell) -> void override;
  auto RestoreShiftedCell(services::Point at, services::CellImage cell) -> void override;
  auto CopyCellBlock(services::Point from, services::Point to) -> void override;
  auto OverlayCell(services::Cell cell, services::CellImage image) -> void override;
  auto MaskCell(services::Cell cell, services::CellMask mask) -> void override;
  auto DrawCellGlyph(services::Cell cell, services::CellGlyph glyph, services::Point at) -> void override;
  auto DrawColouredCellGlyph(services::Cell cell, services::CellGlyph glyph, services::ColouredPlace place) -> void override;
  // ---- sprites: the composing buffer is left alone (the game never reads it)
  auto ComposeSprite(services::Area area, services::Sprite const& sprite) -> void override;
  auto ComposeShiftedSprite(services::Area area, services::Sprite const& sprite) -> void override;
  // ---- handles
  auto Save(services::Area area) -> services::SnapshotId override;
  auto Restore(services::SnapshotId snapshot, services::Point at) -> void override;
  auto TakeCell(services::Point at) -> services::CellId override;
  auto TakeShiftedCell(services::Point at) -> services::CellId override;
  auto MaskInCell(services::CellId cell, services::CellMask mask) -> void override;
  auto OverlayInCell(services::CellId cell, services::CellImage image) -> void override;
  auto GlyphInCell(services::CellId cell, services::CellGlyph glyph, services::Point at) -> void override;
  auto ColouredGlyphInCell(services::CellId cell, services::CellGlyph glyph, services::ColouredPlace place) -> void override;
  auto PutCell(services::Point at, services::CellId cell) -> void override;
  auto PutShiftedCell(services::Point at, services::CellId cell) -> void override;
  auto DrawSprite(services::Area area, services::SpriteImage const& sprite) -> void override;
  auto DrawShiftedSprite(services::Area area, services::SpriteImage const& sprite) -> void override;
  auto DrawStamp(services::Area area, services::Stamp const& stamp) -> void override;
  auto DrawShiftedStamp(services::Area area, services::Stamp const& stamp) -> void override;
  auto ShowOverlay(services::Overlay const& overlay) -> services::OverlayId override;
  auto HideOverlay(services::OverlayId overlay) -> void override;
  // The frame to the renderer's output (black while the screen shows text).
  auto Present() -> void override;

  // ---- the platform's side
  // The frame as Present shows it, 640x480 RGBA.
  [[nodiscard]] auto Capture() -> assets::Texture;
  // The frame as palette entries and every entry's 8-bit levels: what the services' indexed present takes, until the
  // pump presents through Present (the only call that reads the frame back, and only after a change).
  // Whether the frame or its colours changed since the previous call (the frame is painted and read only then).
  auto               RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> frame, std::span<std::uint8_t, PALETTE_LEVELS> palette) -> bool;
  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics {
    return _frame.Statistics();
  }
  // How many steps the frame's recipe holds: what a palette change paints again.
  [[nodiscard]] auto RecipeSize() const noexcept -> std::size_t {
    return _frame.Steps().Steps().size();
  }

private:
  enum class Mode : std::uint8_t {
    TEXT,
    GRAPHICS,
  };
  // A rectangle kept: what was drawn over it (frame pixels).
  struct Kept {
    Recipe recipe;
    Rect   area;
  };
  // What RenderIndexed last gave.
  struct Shown {
    std::array<std::uint8_t, PALETTE_LEVELS> levels{};
    Mode                                     mode{ Mode::TEXT };
  };
  // A battle cell being composed: what was under it, then what is drawn over that, at its place when taken.
  struct Composed {
    Recipe recipe;
    Rect   area;
  };

  auto               Reset(Mode mode) -> void;
  auto               ChangeColours() -> void;
  [[nodiscard]] auto DrawColumnImage(services::Area area, services::Image image, std::size_t planes) -> bool;
  auto               DrawPen(SharedBits const& bits, Rect destination) -> void;
  auto               Exchange(RemapStep step) -> void;
  auto               DrawPicture(services::Point at, PictureByteReader const& reader, assets::Layout stream) -> bool;
  auto               DrawLine(services::Corners ends, Word colour) -> void;
  auto               DrawStampAt(Rect destination, services::Stamp const& stamp) -> void;
  auto               DrawSpriteAt(Rect cut, SharedImage const& image, services::SpriteImage const& sprite) -> void;
  auto               AddAll(std::vector<Step> steps) -> void;
  [[nodiscard]] auto TakeCellAt(Rect area) -> services::CellId;
  auto               PutCellAt(Point at, services::CellId cell) -> void;
  auto               AddToCell(services::CellId cell, std::vector<Step> steps) -> void;
  [[nodiscard]] auto NextOverlay() -> std::uint32_t;

  ScreenPalette                                     _palette;
  GameFrame                                         _frame;
  BitPen                                            _pen;
  Mode                                              _mode{ Mode::GRAPHICS };
  Mode                                              _mode_before{ Mode::TEXT }; // the mode EnterGraphics left
  services::KeptHandles<services::SnapshotId, Kept> _kept{ services::KEPT_SNAPSHOTS };
  services::KeptHandles<services::CellId, Composed> _cells{ services::KEPT_CELLS };
  std::uint32_t                                     _last_overlay{};
  Shown                                             _shown;
};

}

namespace opeu::render {
using detail::render_display::FRAME_PIXELS;
using detail::render_display::RenderDisplay;
}
