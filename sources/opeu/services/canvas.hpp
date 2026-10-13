// The screen the game and the opening draw on: 640x480 pixels of a 16-colour index, the colour map from those
// indices to palette entries, the 256-entry palette in 6-bit levels, and the drawing primitives of the game's library.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <expected>
#include <functional>
#include <span>

namespace opeu::services::detail::canvas {

inline constexpr auto WIDTH{ 640 };
inline constexpr auto HEIGHT{ 480 };
inline constexpr auto PIXELS{ WIDTH * HEIGHT };
inline constexpr auto GAME_TOP{ 40 }; // the game's line 0 is screen line 40; its 400 lines end at 440
inline constexpr auto GAME_LINES{ 400 };
inline constexpr auto COLOURS{ 16 };
inline constexpr auto PALETTE_BYTES{ 256 * 3 };

using Pixel = std::uint8_t; // a colour 0..15

// A pixel position as one number, row-major from the top-left corner: x past the right edge continues on the next
// line, past the store (STORED) it wraps to the start.
using Offset = std::int64_t;

// The canvas stores 819.2 lines: the screen and the lines below it, which the game's unclipped routines may reach. An
// offset past the store wraps to its start, so out-of-range coordinates land where the game's 16-bit screen addresses
// (80 bytes of 8 pixels per line) take them.
inline constexpr Offset STORED{ Offset{ 1 } << 19 };

[[nodiscard]] constexpr auto At(std::int64_t x, std::int64_t line) noexcept -> Offset {
  return line * WIDTH + x;
}

// The offset of pixel x on the game's line y.
[[nodiscard]] constexpr auto GameAt(std::int64_t x, std::int64_t y) noexcept -> Offset {
  return At(x, y + GAME_TOP);
}

// Why a drawing call drew nothing.
enum class DrawError : std::uint8_t {
  OUTSIDE,    // the rectangle is empty or leaves the game area
  BAD_MODE,   // the mode word names no operation
  BAD_HEADER, // a picture's size or position is out of range
};
using Drawn = std::expected<void, DrawError>;

// Supplies the next byte of a picture stream (injected by the caller: a file, a memory block, a test's bytes).
using ByteReader = std::function<std::uint8_t()>;

// Images arrive in the game's forms: column images of one byte per colour bit for each 8 pixels (leftmost in bit 7),
// 1-bit glyphs, pictures in 4-pixel units. Coordinates are game coordinates; the column routines count x and w in
// 8-pixel columns. The mode words are the game's (vgalib.hpp).
class Canvas {
public:
  Canvas() noexcept;

  // ---- modes and colours ----
  // Start-up: shown, black, the start colours, no pen, no remembered mode.
  auto Start() noexcept -> void;
  // Enters graphics (black, the start colours), remembering whether graphics was shown.
  auto EnterGraphics() noexcept -> void;
  // Returns to the remembered mode, black with the start colours; text shows a blank screen.
  auto RestoreMode() noexcept -> void;
  auto MapColour(std::uint8_t colour, std::uint8_t entry) noexcept -> void;
  auto SetPaletteEntry(std::uint8_t entry, std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept -> void;

  // ---- pixels ----
  [[nodiscard]] auto Row(int line) const noexcept -> std::span<Pixel const, WIDTH>;
  [[nodiscard]] auto Pixel(Offset at) const noexcept -> canvas::Pixel {
    return _pixels[Wrap(at)];
  }
  auto Put(Offset at, canvas::Pixel colour) noexcept -> void {
    _pixels[Wrap(at)] = static_cast<canvas::Pixel>(colour & 0x0F);
    _changed          = true;
  }
  // Copies `count` pixels as memmove does (the source read before it is overwritten).
  auto Move(Offset to, Offset from, std::int64_t count) noexcept -> void;
  // The same for a short run of COUNT pixels, inline: the block routines move a few octets per line, so the call and
  // the general run arithmetic would cost more than the copy. A run across the end of the store takes the general path.
  template <std::size_t COUNT>
  auto Move(Offset to, Offset from) noexcept -> void {
    auto const target{ Wrap(to) };
    auto const source{ Wrap(from) };
    if (target > STORED - COUNT || source > STORED - COUNT) return Move(to, from, COUNT);
    std::array<canvas::Pixel, COUNT> held{};
    std::ranges::copy(std::span{ _pixels }.subspan(source).template first<COUNT>(), held.begin());
    std::ranges::copy(held, std::span{ _pixels }.subspan(target).begin());
    _changed = true;
  }
  auto Clear() noexcept -> void;
  // Bit `plane` of the 8 pixels from `at`, the leftmost in bit 7: the form of the game's save buffers.
  [[nodiscard]] auto PlaneBits(Offset at, int plane) const noexcept -> std::uint8_t;
  // The 8 pixels from `at` as one byte per colour bit, and back.
  [[nodiscard]] auto Column(Offset at) const noexcept -> std::array<std::uint8_t, 4>;
  auto               SetColumn(Offset at, std::array<std::uint8_t, 4> const& bits) noexcept -> void;
  // Sets bit `plane` of the 8 pixels from `at` to `bits` where `mask` has a 1.
  auto SetPlaneBits(Offset at, int plane, std::uint8_t bits, std::uint8_t mask = 0xFF) noexcept -> void;

  // ---- primitives ----
  auto DrawColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::span<std::uint8_t const> image, int planes) noexcept -> Drawn;
  auto DrawBits(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::span<std::uint8_t const> bits) noexcept -> Drawn;
  auto DrawBitsOnPaper(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::span<std::uint8_t const> bits) noexcept -> Drawn;
  auto DrawShiftedBits(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::span<std::uint8_t const> bits) noexcept -> Drawn;
  auto DrawShiftedColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::span<std::uint8_t const> image) noexcept -> void;
  auto SaveColumns(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::span<std::uint8_t> image) const noexcept -> void;
  auto Box(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t colour) noexcept -> Drawn;
  auto Highlight(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t colour) noexcept -> Drawn;
  auto Fill(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t colour) noexcept -> void;
  auto Plot(std::uint16_t x, std::uint16_t y, std::uint16_t colour) noexcept -> void;
  auto Line(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t colour) noexcept -> void;
  auto CopyColumns(std::uint16_t x1, std::uint16_t y1, std::uint16_t w, std::uint16_t h, std::uint16_t x2, std::uint16_t y2) noexcept -> void;
  auto DecodePicture(std::uint16_t x, std::uint16_t y, ByteReader const& read, bool masked) noexcept -> Drawn;

  // ---- presentation ----
  // True once after every change.
  [[nodiscard]] auto TakeChanged() noexcept -> bool;
  // The frame as palette entries (all 0 while text is shown) and the palette in 8-bit levels.
  auto               Render(std::span<std::uint8_t, PIXELS> frame, std::span<std::uint8_t, PALETTE_BYTES> palette) const noexcept -> void;
  [[nodiscard]] auto Palette() const noexcept -> std::span<std::uint8_t const, PALETTE_BYTES> {
    return _palette;
  }

private:
  // What a 1 bit (and, for AND, a 0 bit) of the 1-bit routines does to the pixel. The pen stays selected between
  // calls: an operation code those routines do not know draws with the previous pen; at start there is none.
  enum class PenOp : std::uint8_t {
    NONE,
    SET,
    XOR,
    AND,
    OR
  };
  struct Pen {
    PenOp        op{ PenOp::NONE };
    std::uint8_t colour{ 7 };

    // The 8 pixels after drawing the bits of `bits` on `pixels`.
    [[nodiscard]] auto Apply(std::uint64_t pixels, std::uint8_t bits) const noexcept -> std::uint64_t;
  };

  auto                                Reset(bool shown) noexcept -> void;
  [[nodiscard]] static constexpr auto Wrap(Offset at) noexcept -> std::size_t {
    return static_cast<std::size_t>(at & (STORED - 1));
  }
  // `count` pixels from `at` (wrapped), cut at the end of the store.
  [[nodiscard]] auto Run(Offset at, std::int64_t count) noexcept -> std::span<canvas::Pixel> {
    auto const start{ Wrap(at) };
    return std::span{ _pixels }.subspan(start, static_cast<std::size_t>(std::min<Offset>(count, STORED - Offset(start))));
  }
  [[nodiscard]] auto Run(Offset at, std::int64_t count) const noexcept -> std::span<canvas::Pixel const> {
    auto const start{ Wrap(at) };
    return std::span{ _pixels }.subspan(start, static_cast<std::size_t>(std::min<Offset>(count, STORED - Offset(start))));
  }
  // Calls `next(i, pixels)` for the `count` octets from `at` and stores what it returns.
  template <typename _Next>
  auto EachOctet(Offset at, std::size_t count, _Next next) noexcept -> void;
  // The 8 pixels from `at` as the bytes of a word, the leftmost lowest (pixels outside the screen read 0 and are
  // not written).
  [[nodiscard]] auto Load(Offset at) const noexcept -> std::uint64_t;
  auto               Store(Offset at, std::uint64_t pixels) noexcept -> void;
  auto               SelectPen(std::uint8_t op, std::uint8_t colour) noexcept -> void;
  auto               PenBits(Offset at, std::uint8_t bits) noexcept -> void;
  auto               PlotPatterned(std::uint16_t x, std::uint16_t y, std::uint16_t colour, std::uint8_t& pattern_bit) noexcept -> void;
  auto               PutUnit(std::uint16_t unit, std::uint16_t value) noexcept -> void;
  [[nodiscard]] auto MaskedLiteral(std::uint8_t command, ByteReader const& read) const noexcept -> std::uint16_t;

  std::array<canvas::Pixel, STORED>       _pixels{};
  std::array<std::uint8_t, PALETTE_BYTES> _palette{};
  std::array<std::uint8_t, COLOURS>       _colour_map{};
  bool                                    _shown{};
  bool                                    _graphics_before{}; // the mode EnterGraphics left
  bool                                    _changed{ true };
  Pen                                     _pen{};
  std::array<std::uint16_t, 8>            _picture_runs{}; // the masked pictures' run-colour table, kept between pictures
};

// The one screen of the running program. Transitional: the game's drawing calls reach the canvas through this until
// they take it from their caller (#14). Inline, so a call costs a load and a branch.
[[nodiscard]] inline auto Screen() noexcept -> Canvas& {
  static Canvas screen;
  return screen;
}

}

namespace opeu::services {
using detail::canvas::At;
using detail::canvas::ByteReader;
using detail::canvas::Canvas;
using detail::canvas::Drawn;
using detail::canvas::DrawError;
using detail::canvas::GameAt;
using detail::canvas::Offset;
using detail::canvas::Screen;
}
