/* Routines of core/rqsupply.c reached by identifier:
 *   22f9:0136 request_supplies: request-menu handler table DS:a01e
 *             (rqmenu service 30), no arguments; its host entry takes a
 *             scratch region (map-pick bytes at +0/+1, a 50-byte frame
 *             buffer at +4).
 *   22f9:01f2 rq_type_selectable: list-dialog row predicate far pointer
 *             (data/dgsupply.c); takes nothing, byte result in AL. */
#include "core/c2_common.h"

#define RQSUPPLY_SCRATCH 0x40u

PORT_ROUTINE(opeu_request_supplies)
{
    uint16_t scratch;
    (void)args; (void)count;
    scratch = c2_scratch_enter(RQSUPPLY_SCRATCH);
    opeu_request_supplies(ds, link_rqsupply, scratch);
    c2_scratch_leave(RQSUPPLY_SCRATCH);
    return 0;
}

PORT_ROUTINE(opeu_rq_type_selectable)
{
    (void)ds; (void)args; (void)count;
    return opeu_rq_type_selectable();
}
