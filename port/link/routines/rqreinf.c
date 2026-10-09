/* Routines of units/rqreinf.c reached by identifier: 22f9:0780
 * opeu_request_regiment, request-menu choice 2, called through the far
 * handler table DS:a01e (rqmenu service 30, no arguments). */
#include "link.h"

PORT_ROUTINE(opeu_request_regiment)
{
    (void)args; (void)count;
    opeu_request_regiment(ds, link_rqreinf);
    return 0;
}
