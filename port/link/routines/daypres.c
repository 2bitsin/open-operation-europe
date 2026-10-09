/* Routines of core/daypres.c reached by identifier: 0e36:0000 is the far
 * presenter DAILY passes to daily_show (called back through DIALOGS'
 * present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_daily_present)
{
    uint16_t sp;
    (void)args; (void)count;
    /* The 100-byte text lives at BP-64h (sp-66h). */
    sp = port_stack_enter(PORT_FRAME);
    opeu_daily_present(ds, link_daypres, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
