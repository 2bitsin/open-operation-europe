/* Routine of units/orddefnd.c reached by identifier: the defend handler of
 * the DGBATTLE far-code order table (DS:c0c8, finalact service 9). No
 * arguments; returns AX. Takes its BP (entry SP - 2). */
#include "link.h"

PORT_ROUTINE(opeu_order_defend_run)
{
    uint16_t sp, r;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    r = opeu_order_defend_run(ds, link_orddefnd, (uint16_t)(sp - 2));
    port_stack_leave(PORT_FRAME);
    return r;
}
