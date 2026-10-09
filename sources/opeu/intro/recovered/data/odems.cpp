/* OPEN file-backed DGROUP DS:0178..0181 (10 bytes): expanded-memory state.
 *
 * opeu_ems_device_name is the EMM device name that MAIN's EMSDRV object
 * (src/main/asm/emsdrv.asm, linked into OPEN at 031c) compares with the
 * INT 67h device header; it has no terminating NUL (eight characters in an
 * eight-byte array). opeu_sprite_mapped (OEMSMAP, src/open/core/oemsmap.c;
 * the opening tests it before releasing the sprite pages) starts at 0.
 */
#include "types.hpp"

char opeu_ems_device_name[8] = { 'E', 'M', 'M', 'X', 'X', 'X', 'X', '0' };  /* DS:0178 */
int16_t opeu_sprite_mapped = 0;                /* DS:0180 */
