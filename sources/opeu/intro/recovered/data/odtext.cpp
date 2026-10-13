/* OPEN file-backed DGROUP DS:0092..0177 (230 bytes).
 *
 * The first initialized data of the opening's C code, right after the KOEI
 * library _DATA (OFILEIO DS:0042, OVGALIB 006e, OSYSLIB 008a, OVGADEC 008e):
 * a 10-glyph digit font, a far table of two VGA drawing routines, and the
 * variables of the text console at code segment 02a0 (core/otext.cpp), which
 * follows MAIN's TEXTOUT data with OPEN's shorter state. The zero bytes
 * DS:0163 and DS:0173 before opeu_text_large and opeu_text_state are
 * explicit one-byte objects, so that every byte of the range comes from this
 * source. Far pointers are 32-bit segment:offset words.
 */
#include "recovered.hpp"

namespace opeu::intro::detail::odtext {
namespace {
constexpr auto FarCode(uint16_t segment, uint16_t offset) -> uint32_t {
  return static_cast<uint32_t>(segment) << 16 | offset;
}
}

/* DS:0092, 10 x 16 bytes: 8x16 digit glyphs '0'..'9' (one byte per row).
 * No recovered OPEN routine addresses it. */
uint8_t const opeu_open_digit_font[10][16] = {
  { 0x00, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00 },
  { 0x00, 0x18, 0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00 },
  { 0x00, 0x18, 0x24, 0x66, 0x66, 0x66, 0x06, 0x0c, 0x08, 0x10, 0x20, 0x20, 0x60, 0x62, 0x7e, 0x00 },
  { 0x00, 0x18, 0x24, 0x66, 0x66, 0x66, 0x04, 0x18, 0x04, 0x06, 0x66, 0x66, 0x66, 0x24, 0x18, 0x00 },
  { 0x00, 0x0c, 0x1c, 0x1c, 0x1c, 0x2c, 0x2c, 0x2c, 0x4c, 0x4c, 0x4c, 0x7e, 0x0c, 0x0c, 0x0c, 0x00 },
  { 0x00, 0x7c, 0x60, 0x60, 0x60, 0x60, 0x78, 0x64, 0x46, 0x06, 0x06, 0x66, 0x66, 0x24, 0x18, 0x00 },
  { 0x00, 0x18, 0x24, 0x66, 0x66, 0x60, 0x78, 0x64, 0x66, 0x66, 0x66, 0x66, 0x66, 0x24, 0x18, 0x00 },
  { 0x00, 0x7e, 0x66, 0x66, 0x06, 0x04, 0x04, 0x08, 0x08, 0x08, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00 },
  { 0x00, 0x18, 0x24, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x66, 0x66, 0x66, 0x66, 0x66, 0x24, 0x18, 0x00 },
  { 0x00, 0x18, 0x24, 0x66, 0x66, 0x66, 0x66, 0x66, 0x26, 0x1e, 0x06, 0x66, 0x66, 0x24, 0x18, 0x00 }
};

/* DS:0132: the pair {0000:0680 bitmap draw, 0000:069d clipped draw} twice
 * (MAIN's setup data DS:44ec starts with the same pair). The four segment
 * words carry the MZ relocations at image 0x94d4/94d8/94dc/94e0. */
uint32_t const opeu_open_draw_hooks[4] = { FarCode(0x0000, 0x0680), FarCode(0x0000, 0x069d), FarCode(0x0000, 0x0680), FarCode(0x0000, 0x069d) };

/* DS:0142: backspace, ESC M 0 (paper 0), blank, backspace; printed by
 * opeu_text_erase_char. The array is 8 bytes (one NUL of padding). */
uint8_t const opeu_text_erase[8] = "\b\033M0 \b";

/* DS:014a..0159: zero bytes no recovered routine addresses. */

int16_t const opeu_text_x          = 0;    /* DS:015a column 0..79 */
int16_t const opeu_text_y          = 0;    /* DS:015c pixel row */
int16_t const opeu_text_left       = 0;    /* DS:015e left column (ESC R) */
uint8_t const opeu_text_paper      = 0x00; /* DS:0160 */
uint8_t const opeu_text_ink        = 0x07; /* DS:0161 */
uint8_t const opeu_text_background = 0x00; /* DS:0162 */
int16_t const opeu_text_large      = 0;    /* DS:0164 large mode (ESC Y 1) */

/* DS:0166..0171: a saved copy of the console state in MAIN's order (x, y,
 * left, paper, ink 7, background); OPEN never saves or restores it. */
TextSaved const opeu_text_saved = { 0, 0, 0, 0x00, 0x07, 0x00, 0x00, 0 };

uint8_t const  opeu_text_lead     = 0x00; /* DS:0172 pending Shift-JIS lead byte */
int16_t const  opeu_text_state    = 0;    /* DS:0174 escape-sequence state */
uint16_t const opeu_format_output = 0;    /* DS:0176 opeu_request_format destination */

}
