// What the game draws with: the screen's drawing library (vgalib.hpp) and the battle-cell and sprite routines
// (syslib-gfx.hpp). Coordinates are game coordinates (line 0 = screen line 40); column routines take x and width in
// 8-pixel columns. The image formats are those of vgalib.hpp and syslib-gfx.hpp.
#pragma once

#include "machine-types.hpp"
#include "syslib-gfx.hpp"
#include "vgalib.hpp"

#include <cstdint>
#include <span>
#include <variant>

namespace opeu::services::detail::machine_display {

// A rectangle: its top-left place and its size.
struct Area {
  std::uint16_t x{};
  std::uint16_t y{};
  std::uint16_t width{};
  std::uint16_t height{};
};

// Two places: a line's ends, or the inclusive corners of a fill.
struct Corners {
  Point from;
  Point to;
};

// An image and the mode word it is drawn with (operation and colour, per routine).
struct Image {
  std::uint16_t                 mode{};
  std::span<std::uint8_t const> bytes;
};

// A glyph's place in a cell buffer (pixels) and its colour word.
struct ColouredPlace {
  Point         at;
  std::uint16_t colour{};
};

// A sprite composed through a buffer: the AND mask, the interleaved image (3 bytes per column, 4 when flags bit 0 is
// set) and the scratch buffer the screen rectangle is saved into.
struct Sprite {
  std::span<std::uint8_t const> and_mask;
  std::span<std::uint8_t const> or_image;
  std::span<std::uint8_t>       buffer;
  std::uint16_t                 flags{};
};

inline constexpr auto CELL_MASK_BYTES{ 130uz };
inline constexpr auto CELL_GLYPH_LINES{ 6uz };

// A sprite drawn over the screen: where the AND mask's bits are 0 the screen is cleared, then the interleaved image (3
// bytes per column, 4 when flags bit 0 is set) is ORed in.
struct SpriteImage {
  std::span<std::uint8_t const> and_mask;
  std::span<std::uint8_t const> or_image;
  decltype(Sprite::flags)       flags{};
};

// How many saved rectangles, battle cells and overlays a Display keeps at least (the game holds a handful at once).
inline constexpr auto KEPT_SNAPSHOTS{ 64uz };
inline constexpr auto KEPT_CELLS{ 16uz };
inline constexpr auto KEPT_OVERLAYS{ 64uz };

// A one-bit shape stamped over the screen: the pixels under the cut's 0 bits become colour 0, then the ink's 1 bits
// take the colour (a cell frame, an objective's frame); without ink, the cut alone (a shadow). Both are rows of the
// area's whole bytes.
struct Stamp {
  std::span<std::uint8_t const> cut;
  std::span<std::uint8_t const> ink;
  std::uint8_t                  colour{};
};

// A rectangle of the screen the Display keeps for the game to put back.
enum class SnapshotId : std::uint32_t {
};
// A battle cell the Display composes: the screen under it when taken, then the sprites and glyphs drawn over that.
enum class CellId : std::uint32_t {
};
// Something the Display shows over the screen until it is hidden.
enum class OverlayId : std::uint32_t {
};

// The shapes of an overlay. A selection bar over a row or field: x and width in 8-pixel columns.
struct Bar {
  Area columns;
};
// A cursor's shape: x and width in pixels, rows of whole bytes (leftmost pixel in bit 7); its 1 bits are shown.
struct Mark {
  Area                          pixels;
  std::span<std::uint8_t const> bits;
};
// A line (a tracer, a drag outline): its ends in pixels and its dash pattern (0 solid; else bit 7 first, the gaps not
// shown).
struct Trace {
  Corners      ends;
  std::uint8_t dashes{};
};

// An overlay: the pixels its shape covers show the colour of their number with the `exchange` bits flipped (number ^
// exchange, through the palette, as the game's XOR showed them); hidden, they are as they were.
struct Overlay {
  std::variant<Bar, Mark, Trace> shape;
  std::uint8_t                   exchange{};
};

using Cell      = std::span<std::uint8_t, SYSLIB_CELL_BYTES>;
using CellImage = std::span<std::uint8_t const, SYSLIB_CELL_BYTES>;
using CellMask  = std::span<std::uint8_t const, CELL_MASK_BYTES>;
using CellGlyph = std::span<std::uint8_t const, CELL_GLYPH_LINES>;

class Display {
public:
  Display()                                  = default;
  Display(Display const&)                    = delete;
  Display(Display&&)                         = delete;
  auto operator=(Display const&) -> Display& = delete;
  auto operator=(Display&&) -> Display&      = delete;
  virtual ~Display()                         = default;

  // ---- the screen
  virtual auto EnterGraphics() -> void                                                       = 0;
  virtual auto LeaveGraphics() -> void                                                       = 0;
  virtual auto Clear() -> void                                                               = 0;
  virtual auto ShowIdentityPalette() -> void                                                 = 0;
  virtual auto LoadPalette(std::uint8_t first, std::span<std::uint8_t const> levels) -> void = 0;
  // ---- images; true when drawn (inside the game area, a known mode)
  virtual auto DrawColumns(Area area, Image image) -> bool                                = 0;
  virtual auto DrawThreeByteColumns(Area area, Image image) -> bool                       = 0;
  virtual auto DrawBits(Area area, Image image) -> bool                                   = 0;
  virtual auto DrawBitsOnPaper(Area area, Image image) -> void                            = 0;
  virtual auto DrawShiftedBits(Area area, Image image) -> bool                            = 0;
  virtual auto DrawShiftedColumns(Area area, std::span<std::uint8_t const> image) -> void = 0;
  virtual auto SaveColumns(Area area, std::span<std::uint8_t> into) -> void               = 0;
  virtual auto CopyColumns(Area from, Point to) -> void                                   = 0;
  virtual auto DecodePicture(Point at, PictureByteReader const& reader) -> bool           = 0;
  virtual auto DecodeMaskedPicture(Point at, PictureByteReader const& reader) -> bool     = 0;
  // ---- shapes; the colour word as vgalib.hpp describes it per routine
  virtual auto Box(Area area, std::uint16_t colour) -> bool        = 0;
  virtual auto Highlight(Area area, std::uint16_t colour) -> bool  = 0;
  virtual auto Line(Corners ends, std::uint16_t colour) -> void    = 0;
  virtual auto Fill(Corners corners, std::uint16_t colour) -> void = 0;
  // ---- battle cells (x in columns, y in lines; "shifted": half a column right)
  virtual auto SaveCell(Point at, Cell into) -> void                = 0;
  virtual auto SaveShiftedCell(Point at, Cell into) -> void         = 0;
  virtual auto RestoreCell(Point at, CellImage cell) -> void        = 0;
  virtual auto RestoreShiftedCell(Point at, CellImage cell) -> void = 0;
  virtual auto CopyCellBlock(Point from, Point to) -> void          = 0;
  // ---- cell buffers in memory (glyph places in pixels)
  virtual auto OverlayCell(Cell cell, CellImage image) -> void                                = 0;
  virtual auto MaskCell(Cell cell, CellMask mask) -> void                                     = 0;
  virtual auto DrawCellGlyph(Cell cell, CellGlyph glyph, Point at) -> void                    = 0;
  virtual auto DrawColouredCellGlyph(Cell cell, CellGlyph glyph, ColouredPlace place) -> void = 0;
  // ---- sprites (x and width in columns, or in pixels for the shifted one)
  virtual auto ComposeSprite(Area area, Sprite const& sprite) -> void        = 0;
  virtual auto ComposeShiftedSprite(Area area, Sprite const& sprite) -> void = 0;

  // ---- what the game does with the screen, as handles: the methods above that pass the screen's bytes through the
  // game's memory or combine with them are kept for the call sites still to be switched (docs/rendering.md lists them)
  // A column rectangle kept until the Display needs the room (the KEPT_SNAPSHOTS most recent are kept), and put back
  // at a column and line, as often as asked; an id not kept puts back nothing.
  virtual auto Save(Area area) -> SnapshotId                  = 0;
  virtual auto Restore(SnapshotId snapshot, Point at) -> void = 0;
  // A battle cell (x in columns, y in lines; "shifted": half a column right) taken from the screen, composed and put
  // back; the KEPT_CELLS most recent are kept. Glyph places are pixels in the cell.
  virtual auto TakeCell(Point at) -> CellId                                                   = 0;
  virtual auto TakeShiftedCell(Point at) -> CellId                                            = 0;
  virtual auto MaskInCell(CellId cell, CellMask mask) -> void                                 = 0;
  virtual auto OverlayInCell(CellId cell, CellImage image) -> void                            = 0;
  virtual auto GlyphInCell(CellId cell, CellGlyph glyph, Point at) -> void                    = 0;
  virtual auto ColouredGlyphInCell(CellId cell, CellGlyph glyph, ColouredPlace place) -> void = 0;
  virtual auto PutCell(Point at, CellId cell) -> void                                         = 0;
  virtual auto PutShiftedCell(Point at, CellId cell) -> void                                  = 0;
  // A sprite over the screen (x and width in columns, or in pixels for the shifted one).
  virtual auto DrawSprite(Area area, SpriteImage const& sprite) -> void        = 0;
  virtual auto DrawShiftedSprite(Area area, SpriteImage const& sprite) -> void = 0;
  // A stamp (x and width in columns, or in pixels for the shifted one).
  virtual auto DrawStamp(Area area, Stamp const& stamp) -> void        = 0;
  virtual auto DrawShiftedStamp(Area area, Stamp const& stamp) -> void = 0;
  // A selection bar, a cursor or a tracer shown until hidden (the KEPT_OVERLAYS most recent can be hidden).
  virtual auto ShowOverlay(Overlay const& overlay) -> OverlayId = 0;
  virtual auto HideOverlay(OverlayId overlay) -> void           = 0;
  // ---- the frame: the screen as it stands goes to the player.
  virtual auto Present() -> void = 0;
};

}

namespace opeu::services {
using detail::machine_display::Area;
using detail::machine_display::Bar;
using detail::machine_display::Cell;
using detail::machine_display::CELL_GLYPH_LINES;
using detail::machine_display::CELL_MASK_BYTES;
using detail::machine_display::CellGlyph;
using detail::machine_display::CellId;
using detail::machine_display::CellImage;
using detail::machine_display::CellMask;
using detail::machine_display::ColouredPlace;
using detail::machine_display::Corners;
using detail::machine_display::Display;
using detail::machine_display::Image;
using detail::machine_display::KEPT_CELLS;
using detail::machine_display::KEPT_OVERLAYS;
using detail::machine_display::KEPT_SNAPSHOTS;
using detail::machine_display::Mark;
using detail::machine_display::Overlay;
using detail::machine_display::OverlayId;
using detail::machine_display::SnapshotId;
using detail::machine_display::Sprite;
using detail::machine_display::SpriteImage;
using detail::machine_display::Stamp;
using detail::machine_display::Trace;
}
