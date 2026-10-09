/* Link: io/waitkey.c (0a8e:000c..0069 timed waits). Events, from the host
 * branch's comment (src/main/io/waitkey.c):
 *   0 timer start(ticks)  0000:4052 opeu_fm_wait_start
 *   1 timer busy -> word  0000:405f opeu_fm_busy
 *   2 key available       0000:4066 opeu_turn_input_b
 *   3 read key -> AL      0480:0002 opeu_key_read (TEXTOUT)
 *   4 quit to DOS         2597:0b8c opeu_quit_to_dos (SETTINGS) */
#include "link.h"

uint16_t link_waitkey(uint8_t *ds, uint16_t kind, const uint16_t *args, uint16_t count)
{
    switch (kind) {
    case 0: opeu_fm_wait_start(ARG(0)); break;
    case 1: return opeu_fm_busy();
    case 2: return opeu_turn_input_b();
    case 3: return opeu_key_read(ds, link_textout_host());
    case 4: opeu_quit_to_dos(ds, link_settings); break;
    default: port_unhandled("waitkey", kind, count);
    }
    return 0;
}
