/* OPEN file-backed DGROUP DS:0178..0181 and DS:01ac..0355: data of the
 * opening program (code segment 03ac, core/openmain.cpp).
 *
 * Scene tables (frame rows/heights/capture offsets, the two bobbing curves of
 * the aircraft, explosion, flak and shell animation tables), the flash state
 * and the strings the opening passes to the allocator and the text console.
 * The zero bytes after the two 3-byte slot tables (DS:0227, 024f) are
 * explicit one-byte objects, so every byte of the range comes from this
 * source.
 */
#include "recovered.hpp"

namespace opeu::intro::detail::odopen {

/* DS:0178..0181: reserved bytes (the expanded-memory device name and a zero
 * word), kept for the layout until M4. */
char const    opeu_reserved_ems_name[8]   = { 'E', 'M', 'M', 'X', 'X', 'X', 'X', '0' };
int16_t const opeu_reserved_sprite_mapped = 0;

int16_t const opeu_open_aborted = 0; /* DS:01ac input abort flag */

/* DS:01ae..01ef: per-frame rows, heights and capture-buffer offsets. */
int16_t const  opeu_open_frame_y[11]   = { 196, 196, 100, 100, 100, 100, 100, 164, 228, 196, 196 };
int16_t const  opeu_open_frame_h[11]   = { 96, 96, 192, 192, 192, 192, 192, 128, 64, 96, 96 };
uint16_t const opeu_open_frame_off[11] = { 0x0000, 0x1e00, 0x3c00, 0x7800, 0xb400, 0x0000, 0x3c00, 0x7800, 0xa000, 0xb400, 0xd200 };

/* DS:01f0: bobbing curve of the first aircraft group (pixel rows). */
uint16_t const opeu_open_bob_a[20] = { 80, 83, 85, 87, 90, 90, 87, 85, 83, 80, 80, 77, 75, 73, 70, 70, 73, 75, 77, 80 };

/* DS:0218..024e: explosions (frame steps, palette slots, positions, mask and
 * sprite offsets in the far buffers) and the muzzle-flash palette slots. */
int16_t const  opeu_open_blast_steps[6]   = { 1, 1, 2, 3, 4, 4 };
uint8_t const  opeu_open_blast_slots[3]   = { 0x27, 0x28, 0x29 };
int16_t const  opeu_open_blast_x[3]       = { 32, 29, 15 };
int16_t const  opeu_open_blast_y[3]       = { 64, 168, 104 };
uint16_t const opeu_open_blast_masks[6]   = { 0x0000, 0x11e0, 0x23c0, 0x35a0, 0x4780, 0x5960 };
uint16_t const opeu_open_blast_sprites[6] = { 0x0000, 0x4780, 0x8f00, 0x0000, 0x4780, 0x8f00 };
uint8_t const  opeu_open_flash_slots[3]   = { 0x2b, 0x2c, 0x2d };

/* DS:0250: bobbing curve of the second aircraft group. */
uint16_t const opeu_open_bob_b[20] = { 80, 83, 85, 87, 90, 90, 87, 85, 83, 80, 80, 77, 75, 73, 70, 70, 73, 75, 77, 80 };

/* DS:0278..02e1: flak bursts, gun flips and the six shell frames (position,
 * width in bytes for drawing and saving, height, sprite and save offsets). */
int16_t const  opeu_open_flak_x[4]       = { 14, 68, 40, 26 };
int16_t const  opeu_open_flak_y[4]       = { 80, 160, 30, 250 };
int16_t const  opeu_open_gun_flips[3]    = { 10, 20, 6 };
int16_t const  opeu_open_shell_x[6]      = { 72, 57, 50, 50, 49, 49 };
int16_t const  opeu_open_shell_y[6]      = { 208, 264, 280, 288, 296, 296 };
int16_t const  opeu_open_shell_w[6]      = { 8, 6, 4, 1, 2, 1 };
int16_t const  opeu_open_shell_ws[6]     = { 8, 6, 4, 2, 2, 2 };
int16_t const  opeu_open_shell_h[6]      = { 40, 16, 16, 8, 8, 8 };
uint16_t const opeu_open_shell_sprite[6] = { 0x6b40, 0x6c80, 0x6ce0, 0x6d20, 0x6d28, 0x6d38 };
uint16_t const opeu_open_shell_save[6]   = { 0x0000, 0x0500, 0x0680, 0x0780, 0x07c0, 0x0800 };

int16_t const opeu_open_flash_on    = 0; /* DS:02e2 */
int16_t const opeu_open_flash_phase = 0; /* DS:02e4 */

/* DS:02e6..0355: allocation-failure tags (there is no alloc_err8), the
 * console reset sequence (ESC R blank: left 0; ESC C 7: ink 7; ESC M 0:
 * paper 0; ESC B 0: background 0) and the scenario selection file. */
char const opeu_open_text_alloc_err1[] = "alloc_err1";
char const opeu_open_text_alloc_err2[] = "alloc_err2";
char const opeu_open_text_alloc_err3[] = "alloc_err3";
char const opeu_open_text_alloc_err4[] = "alloc_err4";
char const opeu_open_text_alloc_err5[] = "alloc_err5";
char const opeu_open_text_alloc_err6[] = "alloc_err6";
char const opeu_open_text_alloc_err7[] = "alloc_err7";
char const opeu_open_text_alloc_err9[] = "alloc_err9";
char const opeu_open_text_console[]    = "\033R \033C7\033M0\033B0";
char const opeu_open_name_scesel[]     = "SCESEL.DAT";

}
