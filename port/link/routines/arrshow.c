/* Routines of core/arrshow.c reached by identifier: 1270:0816 is the far
 * presenter ARRIVALS and REQSPAWN pass to daily_show (called back through
 * DIALOGS' present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_arrival_present)
{
    uint16_t sp;
    (void)args; (void)count;
    /* scratch is the 32-byte name buffer at BP-20h (entry SP - 2 - 20h). */
    sp = port_stack_enter(PORT_FRAME);
    opeu_arrival_present(ds, link_arrshow, (uint16_t)(sp - 2 - 0x20));
    port_stack_leave(PORT_FRAME);
    return 0;
}
