/* Routines of core/evspy.c reached by identifier: the visitor event
 * (evreport service 5) and the two far presenters passed to daily_show.
 * No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_spy_present_a)
{
    (void)args; (void)count;
    opeu_spy_present_a(ds, link_evspy);
    return 0;
}

PORT_ROUTINE(opeu_spy_present_b)
{
    (void)args; (void)count;
    opeu_spy_present_b(ds, link_evspy);
    return 0;
}

PORT_ROUTINE(opeu_spy_event)
{
    (void)args; (void)count;
    opeu_spy_event(ds, link_evspy);
    return 0;
}
