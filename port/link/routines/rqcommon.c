/* Routines of core/rqcommon.c reached by identifier: 22f9:009a
 * request_finish, the daily_show presentation callback passed by
 * opeu_request_file (far code pointer 22f9:009a). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_request_finish)
{
    (void)args; (void)count;
    opeu_request_finish(ds, link_rqcommon);
    return 0;
}
