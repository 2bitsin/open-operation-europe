/* Routines of core/evpartis.c reached by identifier: 108c:0396 is the far
 * presenter passed to daily_show; 108c:0402 is called by hourly B
 * (evreport service 5). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_hurricane_present)
{
    (void)args; (void)count;
    opeu_hurricane_present(ds, link_evpartis);
    return 0;
}

PORT_ROUTINE(opeu_partisan_event)
{
    (void)args; (void)count;
    opeu_partisan_event(ds, link_evpartis);
    return 0;
}
