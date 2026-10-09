/* Routines of core/evvisit.c reached by identifier: the visit event
 * (evreport service 5) and its far presenter 108c:02ac passed to
 * daily_show. No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_visit_present)
{
    (void)args; (void)count;
    opeu_visit_present(ds, link_evvisit);
    return 0;
}

PORT_ROUTINE(opeu_visit_event)
{
    (void)args; (void)count;
    opeu_visit_event(ds, link_evvisit);
    return 0;
}
