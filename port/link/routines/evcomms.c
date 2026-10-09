/* Routines of core/evcomms.c reached by identifier: the hourly objective
 * event 11e5:01a4 (evreport service 5). No arguments, no result. It
 * receives SP at entry; its text lives at BP-0x6c. */
#include "link.h"

PORT_ROUTINE(opeu_objective_comms_event)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_objective_comms_event(ds, link_evcomms, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
