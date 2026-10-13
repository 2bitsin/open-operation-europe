// What the game draws with: the screen's drawing library (vgalib.hpp) and the battle-cell and sprite routines
// (syslib-gfx.hpp). Coordinates are game coordinates (line 0 = screen line 40); column routines take x and width in
// 8-pixel columns. The image formats are those of vgalib.hpp and syslib-gfx.hpp.
#pragma once

#include "machine-types.hpp"
#include "syslib-gfx.hpp"
#include "vgalib.hpp"

#include <cstdint>
#include <span>

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
};

}

namespace opeu::services {
using detail::machine_display::Area;
using detail::machine_display::Cell;
using detail::machine_display::CELL_GLYPH_LINES;
using detail::machine_display::CELL_MASK_BYTES;
using detail::machine_display::CellGlyph;
using detail::machine_display::CellImage;
using detail::machine_display::CellMask;
using detail::machine_display::ColouredPlace;
using detail::machine_display::Corners;
using detail::machine_display::Display;
using detail::machine_display::Image;
using detail::machine_display::Sprite;
}
