/* Routines of core/evwound.c reached by identifier:
 * the hourly wound event (evreport service 5) and the far
 * death presenter passed to daily_show. No arguments, no result. The
 * events receive SP at entry (BP = SP-2). */
#include "link.h"

PORT_ROUTINE(opeu_death_present)
{
    (void)args; (void)count;
    opeu_death_present(ds, link_evwound);
    return 0;
}

PORT_ROUTINE(opeu_wound_event)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_wound_event(ds, link_evwound, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
