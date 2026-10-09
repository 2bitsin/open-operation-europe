/* Native C reimplementation of KOEI's planar VGA library (MAIN.EXE
 * src/main/asm/vgalib.asm, _TEXT 23AE..34B7) for the Operation Europe port.
 *
 * Display model
 * -------------
 * The original draws in BIOS mode 12h (640x480, 16 colours, 4 bit planes,
 * 80 bytes per line).  The game area is 400 lines starting at line 40, so
 * every routine addresses VRAM byte (y + 40) * 80 + x_byte.  Several routines
 * work byte-wise per plane (plane-masked writes, latched copies, 4-pixel
 * nibble copies in the picture decoder, rows written past line 440 by
 * row-doubled blits, 16-bit address wrap-around), so VRAM is kept exactly as
 * the hardware holds it: four 64 KiB planes.  Pixel (x, y) of the screen is
 * bit (7 - x % 8) of byte y * 80 + x / 8 in each plane; plane p supplies bit
 * p of the 4-bit colour.  vgalib_render() converts the visible 480 lines to
 * a chunky 640x480 buffer of DAC indices through the attribute palette.
 *
 * No VGA register is emulated: each routine performs on the planes exactly
 * the effect the original's register programming has on real hardware
 * (map mask, latches, bit mask, logical functions, write modes).
 *
 * Pointers: the original far (segment, offset) argument pairs and near DS
 * offsets become plain host pointers.  Coordinates keep the original units
 * (x in bytes for the byte-aligned routines, pixels elsewhere).
 */
#pragma once

#include <stdint.h>

inline constexpr auto VGA_PLANE_SIZE{ 0x10000u };
inline constexpr auto VGA_PITCH{ 80u };
inline constexpr auto VGA_TOP{ 40u }; /* first game line on the 480-line screen */

extern uint8_t vga_plane[4][VGA_PLANE_SIZE];
extern uint8_t vga_dac[256 * 3];     /* 6-bit R,G,B */
extern uint8_t vga_attr[16];         /* attribute palette: pixel -> DAC index */
extern uint8_t vga_pel_mask;
extern int vga_text_mode;        /* 1 while the BIOS text mode is active */
extern int vga_dirty;            /* set by every change to the display */

/* Reset all state to the power-on text mode (call once at start-up). */
void vgalib_init(void);
/* Render the 640x480 screen into `out` (DAC indices) and the palette as 8-bit
 * RGB; returns 1 if anything changed since the previous call.  In text mode
 * the output is blank (index 0). */
int vgalib_render(uint8_t* out640x480, uint8_t pal_rgb8[768]);

/* Reader callback of the picture decoder: returns the next stream byte. */
typedef uint8_t (*VgaByteReader)(void);

/* ---- vgalib.asm entry points (original address in brackets) ------------ */

void vga_reset_gc(void);                                           /* 23AE */
void koei_vga_identity_palette(void);                              /* 23C4 */
int16_t opeu_set_palette(uint8_t first, uint8_t count, const uint8_t* rgb); /* 23D5 */
int16_t koei_266c(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 266C */
int16_t opeu_bitmap_restore(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode,
                            const uint8_t* data); /* 2674 */
int16_t koei_27c2(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 27C2 */
int16_t opeu_blit_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 27CA */
int16_t opeu_draw_near(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 28A8 */
inline constexpr auto& opeu_code_0000_28a8{ opeu_draw_near };
int16_t opeu_draw_far(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 28B0 */
void opeu_code_0000_28c5(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 28C5 */
int16_t opeu_vga_glyph(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 2A81 */
int16_t opeu_gfx_blit(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t mode, const uint8_t* data); /* 2A96 */
inline constexpr auto& opeu_bitmap_draw{ opeu_gfx_blit };
int16_t opeu_vga_clear(void); /* 2AAC */
int16_t opeu_ui_box(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color); /* 2AD2 */
inline constexpr auto& opeu_vga_fill_rect{ opeu_ui_box };
int16_t opeu_ui_highlight(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color); /* 2B0E */
void koei_2ce4(uint16_t x, uint16_t y, uint16_t color); /* 2CE4 */
void opeu_ui_line(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color); /* 2D16 */
void koei_2e49(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data); /* 2E49 */
void koei_2e57(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* data); /* 2E57 */
void koei_2e72(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest); /* 2E72 */
void opeu_bitmap_save(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* dest); /* 2ED8 */
void opeu_ui_fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color); /* 2FA5 */
void koei_2fea(void); /* 2FEA */
void opeu_text_mode(void); /* 300C */
void opeu_screen_copy(uint16_t x1, uint16_t y1, uint16_t w, uint16_t h, uint16_t x2, uint16_t y2); /* 301F */
inline constexpr auto& opeu_vga_copy{ opeu_screen_copy };
int16_t opeu_picture_decode_masked(uint16_t x, uint16_t y, VgaByteReader reader); /* 3490 */
int16_t opeu_picture_decode(uint16_t x, uint16_t y, VgaByteReader reader); /* 34A4 */
