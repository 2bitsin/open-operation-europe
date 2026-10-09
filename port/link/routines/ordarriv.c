/* Routines of units/ordarriv.c reached by identifier: order codes 0, 12 and
 * 13 of the DGBATTLE far-code order table (DS:c0c8, finalact service 9).
 * No arguments; they return AX and take their entry SP. */
#include "link.h"

PORT_ROUTINE(opeu_order_arrive)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_order_arrive(ds, link_ordarriv, sp);
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_order_reach_unit)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_order_reach_unit(ds, link_ordarriv, sp);
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_order_hold)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_order_hold(ds, link_ordarriv, sp);
    port_stack_leave(PORT_FRAME);
    return r;
}
