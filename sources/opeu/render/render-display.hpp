#pragma once
// The game's screen: services::Display (what the game draws with) on the frame of colour numbers the services' native
// display draws (services::NativeDisplay over the Canvas: byte copies, the game's own routines), with the source layer
// beside it (source-layer.hpp): which cells show an image of the pack, recognised from the bytes the game hands over
// (assets::ContentIndex), and which are raw numbers. The pump presents through Present, which hands the frame to the
// platform as palette entries (the window, the pointer, the headless PNG); Capture turns the numbers into colours on
// the CPU when asked, and nothing is ever read back from the renderer. docs/rendering.md is the design, the source
// layer's rule and the remaster present it prepares, which draws on the display's renderer.

#include "opeu/assets/content-index.hpp"
#include "opeu/assets/pack.hpp"
#include "opeu/assets/texture.hpp"
#include "opeu/render/display.hpp"
#include "opeu/render/recognised-images.hpp"
#include "opeu/render/source-layer.hpp"
#include "opeu/services/kept-handles.hpp"
#include "opeu/services/machine-display.hpp"
#include "opeu/services/native-machine.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace opeu::render::detail::render_display {

inline constexpr auto FRAME_PIXELS{ 640uz * 480uz };
inline constexpr auto PALETTE_LEVELS{ 768uz }; // three 8-bit levels per palette entry, what the pump presents

// The drawing library's 16-bit argument word (a coordinate, a count, a colour or pen word).
using Word = decltype(services::Point::x);

class RenderDisplay final : public services::Display {
public:
  // The game's screen, shown on `display`'s renderer. The images whose bytes `index` recognises name their id in the
  // source layer when `pack` holds it; both outlive the display.
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
  // ---- battle cells through the game's memory
  auto SaveCell(services::Point at, services::Cell into) -> void override;
  auto SaveShiftedCell(services::Point at, services::Cell into) -> void override;
  auto RestoreCell(services::Point at, services::CellImage cell) -> void override;
  auto RestoreShiftedCell(services::Point at, services::CellImage cell) -> void override;
  auto CopyCellBlock(services::Point from, services::Point to) -> void override;
  auto OverlayCell(services::Cell cell, services::CellImage image) -> void override;
  auto MaskCell(services::Cell cell, services::CellMask mask) -> void override;
  auto DrawCellGlyph(services::Cell cell, services::CellGlyph glyph, services::Point at) -> void override;
  auto DrawColouredCellGlyph(services::Cell cell, services::CellGlyph glyph, services::ColouredPlace place) -> void override;
  // ---- sprites composed in the game's buffer
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
  // The frame to the player: to the platform as palette entries with the pointer, unless nothing changed.
  auto Present() -> void override;

  // ---- the platform's side
  // The frame as Present shows it, 640x480 RGBA.
  [[nodiscard]] auto Capture() const -> assets::Texture;
  // The frame as palette entries and every entry's 8-bit levels, what Present hands the platform; whether either
  // changed since the previous call.
  auto               RenderIndexed(std::span<std::uint8_t, FRAME_PIXELS> frame, std::span<std::uint8_t, PALETTE_LEVELS> palette) -> bool;
  [[nodiscard]] auto Statistics() const noexcept -> ByteStatistics {
    return _images.Statistics();
  }
  // Where the frame's cells came from.
  [[nodiscard]] auto Sources() const noexcept -> SourceLayer const& {
    return _sources;
  }
  // The memory the display holds besides its fixed frames: the kept sources and the layer's names.
  [[nodiscard]] auto Footprint() const noexcept -> std::size_t;

private:
  // An image the game handed over as bytes, laid at `area` (frame pixels).
  struct Handed {
    std::optional<std::string_view> id; // the pack image, when the bytes are one
    Rect                            area;
    std::uint8_t                    colours{};
  };
  // The last XOR highlight: drawn again over the same place, it puts its cells back.
  struct Highlighted {
    services::Area area;
    Word           colour{};
    Toggled        toggled;
  };

  // The source of `handed`'s cells: its pack image when recognised and packed, else none (the cells it touches raw).
  auto Lay(Handed const& handed) -> void;
  auto LayCut(Handed const& handed, std::span<std::uint8_t const> cut) -> void;
  auto ToggleHighlight(services::Area area, Word colour) -> void;
  auto LayPicture(services::Point at, std::span<std::uint8_t const> stream, assets::Layout layout) -> void;

  render::Display                                          _display;
  RecognisedImages                                         _images;
  services::NativeDisplay                                  _native;
  SourceLayer                                              _sources;
  services::KeptHandles<services::SnapshotId, KeptSources> _kept{ services::KEPT_SNAPSHOTS };
  services::KeptHandles<services::OverlayId, Toggled>      _overlays{ services::KEPT_OVERLAYS };
  std::optional<Highlighted>                               _highlighted;
};

}

namespace opeu::render {
using detail::render_display::FRAME_PIXELS;
using detail::render_display::PALETTE_LEVELS;
using detail::render_display::RenderDisplay;
using detail::render_display::Word;
}
