/* Link: core/halfday.c (0e47:00e0 opeu_turn_half_day). Services:
 * prepare=0, noon events=1. */
#include "link.h"

/* opeu_turn_noon_events keeps a 90-byte buffer at BP-0x62 and nests the
 * leader (BP-0x66-4) and speech (BP-0x78-4) frames below it. */
#define NOON_RESERVE 0x200u

uint16_t link_halfday(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    uint16_t sp;
    (void)a0; (void)a1; (void)a2; (void)a3;
    switch (id) {
    case 0: opeu_turn_prepare(ds, link_dialogs_host()); break;               /* 1707:00c8 */
    case 1:                                                                  /* 0e57:0296 */
        /* far_text: base of the far text segment 3895; the records' far
         * actions are compared with this code segment 0e57. */
        sp = port_stack_enter(NOON_RESERVE);
        opeu_turn_noon_events(ds, port_far(0x3895, 0), link_noonevt, sp, 0x0e57);
        port_stack_leave(NOON_RESERVE);
        break;
    default: port_unhandled("halfday", id, 4);
    }
    return 0;
}
