/* Routines of core/reqerror.c reached by identifier: the daily_show
 * presentation callback 1270:02e4 (far code pointer passed by
 * opeu_request_valid). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_request_error_present)
{
    (void)args; (void)count;
    opeu_request_error_present(ds, link_reqerror);
    return 0;
}
