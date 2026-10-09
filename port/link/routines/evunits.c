/* Routines of core/evunits.c reached by identifier: the hourly unit
 * condition event 11e5:0606 (evreport service 5). No arguments, no
 * result. */
#include "link.h"

PORT_ROUTINE(opeu_unit_hourly_event)
{
    (void)args; (void)count;
    opeu_unit_hourly_event(ds, link_evunits);
    return 0;
}
