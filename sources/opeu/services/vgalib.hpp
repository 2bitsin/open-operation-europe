// The game's drawing library (VGALIB) on the canvas: palette, image blits, 1-bit glyphs, boxes, lines, fills, screen
// save and copy, the picture decoders. Coordinates are game coordinates (line 0 = screen line 40); byte-aligned
// routines take x and w in 8-pixel columns. Images arrive in the game's formats and are decoded at the call.
#pragma once

#include <cstdint>
#include <functional>
#include <span>

// Start-up: the graphics screen shown, cleared, with the start colour map and palette.
auto vgalib_init() -> void;
// The frame as palette entries and the palette in 8-bit levels; true when anything changed since the previous call.
auto VgalibRender(std::span<std::uint8_t, 640 * 480> frame, std::span<std::uint8_t, 768> palette) -> bool;

// Reader of the picture decoders: returns the next stream byte. A callable the caller injects; a plain function
// pointer (the game's port_picture_reader, the opening's stream) converts to it.
using PictureByteReader = std::function<std::uint8_t()>;

// Colours 0-15 show palette entries 0-15.
auto koei_vga_identity_palette() -> void;
// Loads `count` palette entries from `first & 15` (a count above 16 becomes 16 - first; first + count may pass 15;
// 0 loads none). Each source entry is three 4-bit values b0 b1 b2: red b1, green b2, blue b0, scaled by 4. Returns 0.
auto opeu_set_palette(std::uint8_t first, std::uint8_t count, std::uint8_t const* rgb) -> std::int16_t;

// Image of w columns x h lines, each column 4 bytes (bit p of the colours, leftmost pixel in bit 7). mode & 7FFF:
// 0 copy, 1 XOR, 2 AND, 3 OR, 4 transparent copy (colour 0 keeps the screen). Mode 0 with bit 15 draws every row on
// two lines (the clip checks h only). Returns 0, or -1 for a rectangle outside the game area or a mode above 4.
auto koei_266c(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
auto opeu_bitmap_restore(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
// The same with 3 bytes per column (colour bit 3 not stored). mode & 3FFF: 0 copy with bit 3 = 0 (1 with bit 14);
// 1 XOR, 2 AND, 3 OR on bits 0-2 (bit 3 kept); 4 transparent copy that clears bit 3 under the drawn pixels.
auto koei_27c2(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
auto opeu_blit_far(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
// 1-bit image of w bytes per row. Low byte of mode: the colour; (mode & 7FFF) >> 8: 0 opaque (0 bits draw colour 0),
// 4 set, 1 XOR, 2 AND (0 bits clear the colour's bits), 3 OR; 5-7F repeat the previous pen. Bit 15 doubles rows.
auto                   opeu_draw_near(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
inline constexpr auto& CODE_0000_28A8{ opeu_draw_near };
auto                   opeu_draw_far(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
// 1-bit glyph on paper: 1 bits colour (mode & 15), 0 bits colour ((mode >> 8) & 15).
auto opeu_code_0000_28c5(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> void;
// 1-bit image at any pixel x (x and w in pixels, rows of ceil(w / 8) bytes, whole source bytes drawn). High byte of
// mode: 0 or 4 set, 1 XOR, 2 AND, 3 OR, others the previous pen; the pixels of the edge columns outside the image
// get a 0 bit (a 1 bit for AND). Returns 0 or -1.
auto                   opeu_vga_glyph(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
auto                   opeu_gfx_blit(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t mode, std::uint8_t const* data) -> std::int16_t;
inline constexpr auto& BITMAP_DRAW{ opeu_gfx_blit };
// Clears the screen to colour 0. Returns 0.
auto opeu_vga_clear() -> std::int16_t;
// Solid box in colour (color & 15). Returns 0 or -1.
auto                   opeu_ui_box(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t color) -> std::int16_t;
inline constexpr auto& opeu_vga_fill_rect{ opeu_ui_box };
// XOR (high byte 1), AND (2: no change) or OR (3) the colour over a box; selects that pen. Returns 0 or -1.
auto opeu_ui_highlight(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint16_t color) -> std::int16_t;
// One pixel in colour (low byte, as ui_line without a pattern).
auto Koei2ce4(std::uint16_t x, std::uint16_t y, std::uint16_t color) -> void;
// Line; colour word: low nibble colour, bits 4-7 operation (0 set, 1 XOR, 2 AND, 3 OR, 4 OR on pattern bits only),
// high byte a dash pattern.
auto opeu_ui_line(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t color) -> void;
// Transparent 4-byte-column image at any pixel x, unclipped (see Canvas::DrawShiftedColumns for its quirk).
auto Koei2e49(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* data) -> void;
auto koei_2e57(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t const* data) -> void;
// Saves a w x h column rectangle in the 4-byte-column format, unclipped.
auto koei_2e72(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t* dest) -> void;
auto opeu_bitmap_save(std::uint16_t x, std::uint16_t y, std::uint16_t w, std::uint16_t h, std::uint8_t* dest) -> void;
// Fills pixels x1..x2, y1..y2 (inclusive) in colour (color & 15), whole 8-pixel columns between the edges.
auto opeu_ui_fill(std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2, std::uint16_t color) -> void;
// Enters graphics: remembers the current mode, then a cleared screen with the start colours.
auto koei_2fea() -> void;
// Returns to the mode remembered by koei_2fea (graphics after start-up: a cleared screen).
auto opeu_text_mode() -> void;
// Copies a column rectangle (x1, y1) -> (x2, y2), w columns x h lines, as memmove in screen order.
auto                   opeu_screen_copy(std::uint16_t x1, std::uint16_t y1, std::uint16_t w, std::uint16_t h, std::uint16_t x2, std::uint16_t y2) -> void;
inline constexpr auto& opeu_vga_copy{ opeu_screen_copy };
// Picture decoders (stream format at Canvas::DecodePicture): masked (run table, 16 colours) and plain (8 colours). 0 or -1.
auto opeu_picture_decode_masked(std::uint16_t x, std::uint16_t y, PictureByteReader const& reader) -> std::int16_t;
auto opeu_picture_decode(std::uint16_t x, std::uint16_t y, PictureByteReader const& reader) -> std::int16_t;
