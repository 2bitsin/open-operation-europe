/* Routines of units/rqarms.c reached by identifier:
 *  - 22f9:03b6 opeu_request_arms, request-menu choice 1, called through the
 *    far handler table DS:a01e (rqmenu service 30, no arguments). Its host
 *    entry takes `scratch`, a DS area for the frame objects (pick x/y at
 *    +0/+1, the 50-byte text at +4), placed in a modeled frame.
 *  - 22f9:04ce opeu_rq_unit_selectable, far row predicate of the regiment
 *    list (src/main/data/dgsupply.c:510). No arguments; returns AL. */
#include "../units/rq_F.h"

PORT_ROUTINE(opeu_request_arms)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(LF_FRAME);
    opeu_request_arms(ds, link_rqarms, (uint16_t)(sp - LF_RQ_SCRATCH));
    port_stack_leave(LF_FRAME);
    return 0;
}

PORT_ROUTINE(opeu_rq_unit_selectable)
{
    (void)ds; (void)args; (void)count;
    return opeu_rq_unit_selectable();
}
