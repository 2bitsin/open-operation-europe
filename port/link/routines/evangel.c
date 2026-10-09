/* Routines of core/evangel.c reached by identifier: the visit event
 * (evreport service 5) and its far presenter passed to daily_show. No
 * arguments, no result. code_segment is 108c in the port's original
 * segment numbering (the DS:7cec action table holds 108c pointers). */
#include "link.h"

PORT_ROUTINE(opeu_angel_present)
{
    (void)args; (void)count;
    opeu_angel_present(ds, link_evangel, 0x108c);
    return 0;
}

PORT_ROUTINE(opeu_angel_event)
{
    (void)args; (void)count;
    opeu_angel_event(ds, link_evangel, 0x108c);
    return 0;
}
