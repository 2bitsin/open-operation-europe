/* OPEN file-backed DGROUP DS:0182..01ab (42 bytes): sound driver state of
 * OSOUND (src/open/io/osound.c, MAIN SOUND lineage).
 *
 * The current music track and effect start at 0ffh (none loaded); the music
 * bank name and path buffers start empty. OSOUND uses 7 and 13 bytes of the
 * two buffers; the arrays extend to the next object, all zero.
 */
#include "types.hpp"

uint8_t opeu_music_current = 0xff;     /* DS:0182 */
uint8_t opeu_effect_current = 0xff;    /* DS:0183 */
char opeu_music_bank[26] = {0};        /* DS:0184 */
char opeu_music_path[14] = {0};        /* DS:019e */
