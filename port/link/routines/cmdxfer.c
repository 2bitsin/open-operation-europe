/* Routines of units/cmdxfer.c reached by identifier: the three list
 * predicates 1d42:009a/00d4/0110 that opeu_turn_action_3 stores as far
 * pointers into the list windows (DS:5700/5848/5906 +15) for the list
 * dialog, and the daily_show presenter 1d42:01ca (far pointer passed with
 * event 7). No arguments; predicates return AL. */
#include "link.h"

PORT_ROUTINE(opeu_transfer_group_enabled)
{
    (void)args; (void)count;
    return opeu_transfer_group_enabled(ds);
}

PORT_ROUTINE(opeu_transfer_unit_enabled)
{
    (void)args; (void)count;
    return opeu_transfer_unit_enabled(ds);
}

PORT_ROUTINE(opeu_transfer_element_enabled)
{
    (void)args; (void)count;
    return opeu_transfer_element_enabled(ds);
}

/* 1d42:01d2, turn action 3, an entry of the control action table DS:7fb4
 * (control1 service 10). No arguments; the host entry takes its entry SP
 * and the paragraph of code segment 1d42 (stored with the predicates). */
PORT_ROUTINE(opeu_turn_action_3)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(0x200u);
    opeu_turn_action_3(ds, link_cmdxfer, sp, 0x1d42u);
    port_stack_leave(0x200u);
    return 0;
}

PORT_ROUTINE(opeu_transfer_present)
{
    (void)args; (void)count;
    opeu_transfer_present(ds, link_cmdxfer);
    return 0;
}
