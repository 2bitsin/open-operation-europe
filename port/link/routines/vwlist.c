/* Routines of units/vwlist.c reached by identifier: the list-dialog row
 * checks (far pointers 1fe6:xxxx that the view modules store into the list
 * dialogs' eligibility slot, see VW_SET_CHECK). No arguments; result AL.
 * The host entries take the entry SP (unused by these checks). */
#include "../units/units_F.h"

/* 1fe6:0120 */
PORT_ROUTINE(opeu_view_objective_ok)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_objective_ok(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:016e */
PORT_ROUTINE(opeu_view_false)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_false(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:0172 */
PORT_ROUTINE(opeu_view_group_ok)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_group_ok(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:01ac */
PORT_ROUTINE(opeu_view_unit_ok)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_unit_ok(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:0200 */
PORT_ROUTINE(opeu_view_false_2)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_false_2(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:0204 */
PORT_ROUTINE(opeu_view_false_3)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_false_3(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}

/* 1fe6:0208 */
PORT_ROUTINE(opeu_view_true)
{
    uint16_t sp = port_stack_enter(PORT_FRAME);
    uint8_t r = opeu_view_true(ds, link_vwlist, sp);
    (void)args; (void)count;
    port_stack_leave(PORT_FRAME);
    return r;
}
