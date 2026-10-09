/* Routines of core/reqpend.c reached by identifier: the daily_show
 * presentation callbacks 1270:0430 and 1270:04f0 (far code pointers passed
 * by opeu_request_start / opeu_request_fallback). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_pending_start_present)
{
    (void)args; (void)count;
    opeu_pending_start_present(ds, link_reqpend);
    return 0;
}

PORT_ROUTINE(opeu_pending_fallback_present)
{
    (void)args; (void)count;
    opeu_pending_fallback_present(ds, link_reqpend);
    return 0;
}
