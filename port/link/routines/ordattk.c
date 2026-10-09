/* Routines of units/ordattk.c reached by identifier: the occupy and attack
 * handlers of the DGBATTLE far-code order table (DS:c0c8, raised by
 * finalact service 9) and the two presenters passed to daily_show (far
 * pointers 3131:00f6/0282, dialogs service 47). No arguments; the handlers
 * return AX and take their BP (entry SP - 2). attack_start nests its
 * callees at BP-72h, hence the larger reserve. */
#include "link.h"

PORT_ROUTINE(opeu_order_occupy_run)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(0x200);
    r = opeu_order_occupy_run(ds, link_ordattk, (uint16_t)(sp - 2));
    port_stack_leave(0x200);
    return r;
}

PORT_ROUTINE(opeu_order_attack_run)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(0x200);
    r = opeu_order_attack_run(ds, link_ordattk, (uint16_t)(sp - 2));
    port_stack_leave(0x200);
    return r;
}

PORT_ROUTINE(opeu_order_attack_present)
{
    (void)args; (void)count;
    opeu_order_attack_present(ds, link_ordattk);
    return 0;
}

PORT_ROUTINE(opeu_order_occupied_present)
{
    (void)args; (void)count;
    opeu_order_occupied_present(ds, link_ordattk);
    return 0;
}
