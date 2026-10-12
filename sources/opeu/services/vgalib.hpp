// The game's drawing library (VGALIB) on the canvas: palette, image blits, 1-bit glyphs, boxes, lines, fills, screen
// save and copy, the picture decoders. Coordinates are game coordinates (line 0 = screen line 40); byte-aligned
// routines take x and w in 8-pixel columns. Images arrive in the game's formats and are decoded at the call.
#pragma once

#include <stdint.h>

// Start-up: the graphics screen shown, cleared, with the start colour map and palette.
void vgalib_init(void);
// The frame as palette entries and the palette in 8-bit levels; 1 when anything changed since the previous call.
int vgalib_render(uint8_t* out640x480, uint8_t pal_rgb8[768]);

// Reader callback of the picture decoders: returns the next stream byte.
using PictureByteReader = uint8_t (*)(void);

// Colours 0-15 show palette entries 0-15.
void koei_vga_identity_palette(void);
// Loads `count` palette entries from `first & 15` (a count above 16 becomes 16 - first; first + count may pass 15;
// 0 loads none). Each source entry is three 4-bit values b0 b1 b2: red b1, green b2, blue b0, scaled by 4. Returns 0.
int16_t opeu_set_palette(uint8_t first, uint8_t count, const uint8_t* rgb);

// Image of w columns x h lines, each column 4 bytes (bit p of the colours, leftmost pixel in bit 7). mode & 7FFF:
// 0 copy, 1 XOR, 2 AND, 3 OR, 4 transparent copy (colour 0 keeps the screen). Mode 0 with bit 15 draws every row on
// two lines (the clip checks h only). Returns 0, or -1 for a rectangle outside the game area or a mode above 4.
int16_t koei_266c(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
int16_t opeu_bitmap_restore(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
// The same with 3 bytes per column (colour bit 3 not stored). mode & 3FFF: 0 copy with bit 3 = 0 (1 with bit 14);
// 1 XOR, 2 AND, 3 OR on bits 0-2 (bit 3 kept); 4 transparent copy that clears bit 3 under the drawn pixels.
int16_t koei_27c2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
int16_t opeu_blit_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
// 1-bit image of w bytes per row. Low byte of mode: the colour; (mode & 7FFF) >> 8: 0 opaque (0 bits draw colour 0),
// 4 set, 1 XOR, 2 AND (0 bits clear the colour's bits), 3 OR; 5-7F repeat the previous pen. Bit 15 doubles rows.
int16_t opeu_draw_near(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
inline constexpr auto& opeu_code_0000_28a8{ opeu_draw_near };
int16_t opeu_draw_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
// 1-bit glyph on paper: 1 bits colour (mode & 15), 0 bits colour ((mode >> 8) & 15).
void opeu_code_0000_28c5(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
// 1-bit image at any pixel x (x and w in pixels, rows of ceil(w / 8) bytes, whole source bytes drawn). High byte of
// mode: 0 or 4 set, 1 XOR, 2 AND, 3 OR, others the previous pen; the pixels of the edge columns outside the image
// get a 0 bit (a 1 bit for AND). Returns 0 or -1.
int16_t opeu_vga_glyph(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
int16_t opeu_gfx_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data);
inline constexpr auto& opeu_bitmap_draw{ opeu_gfx_blit };
// Clears the screen to colour 0. Returns 0.
int16_t opeu_vga_clear(void);
// Solid box in colour (color & 15). Returns 0 or -1.
int16_t                opeu_ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
inline constexpr auto& opeu_vga_fill_rect{ opeu_ui_box };
// XOR (high byte 1), AND (2: no change) or OR (3) the colour over a box; selects that pen. Returns 0 or -1.
int16_t opeu_ui_highlight(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
// One pixel in colour (low byte, as ui_line without a pattern).
void koei_2ce4(uint16_t x, uint16_t y, uint16_t color);
// Line; colour word: low nibble colour, bits 4-7 operation (0 set, 1 XOR, 2 AND, 3 OR, 4 OR on pattern bits only),
// high byte a dash pattern.
void opeu_ui_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
// Transparent 4-byte-column image at any pixel x, unclipped (see Canvas::DrawShiftedColumns for its quirk).
void koei_2e49(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data);
void koei_2e57(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data);
// Saves a w x h column rectangle in the 4-byte-column format, unclipped.
void koei_2e72(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest);
void opeu_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest);
// Fills pixels x1..x2, y1..y2 (inclusive) in colour (color & 15), whole 8-pixel columns between the edges.
void opeu_ui_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
// Enters graphics: remembers the current mode, then a cleared screen with the start colours.
void koei_2fea(void);
// Returns to the mode remembered by koei_2fea (graphics after start-up: a cleared screen).
void opeu_text_mode(void);
// Copies a column rectangle (x1, y1) -> (x2, y2), w columns x h lines, as memmove in screen order.
void                   opeu_screen_copy(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t x2, uint16_t y2);
inline constexpr auto& opeu_vga_copy{ opeu_screen_copy };
// Picture decoders (stream format at Canvas::DecodePicture): masked (run table, 16 colours) and plain (8 colours).
// 0 or -1.
int16_t opeu_picture_decode_masked(uint16_t x, uint16_t y, PictureByteReader reader);
int16_t opeu_picture_decode(uint16_t x, uint16_t y, PictureByteReader reader);
