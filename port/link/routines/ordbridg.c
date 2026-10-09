/* Routines of units/ordbridg.c reached by identifier: the destroy-bridge
 * handler of the DGBATTLE far-code order table (DS:c0c8, finalact service
 * 9; returns AX, takes its BP = entry SP - 2) and its daily_show presenter
 * (far pointer 3131:060c, dialogs service 47). */
#include "link.h"

PORT_ROUTINE(opeu_order_destroy_run)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_order_destroy_run(ds, link_ordbridg, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}

PORT_ROUTINE(opeu_order_destroy_present)
{
    (void)args; (void)count;
    opeu_order_destroy_present(ds, link_ordbridg);
    return 0;
}
