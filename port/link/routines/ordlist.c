/* Routines of units/ordlist.c reached by identifier: 1c23:0e86, turn action
 * 1 (the Orders command), an entry of the control action table DS:7fb4
 * (control1 service 10). No arguments. The host entry takes its entry SP
 * and the paragraph of code segment 1c23 (stored with the list predicate
 * offsets 0006/0040/021c into the list dialogs' far pointers). */
#include "../units/units_F.h"

PORT_ROUTINE(opeu_turn_action_1)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(LF_FRAME);
    opeu_turn_action_1(ds, link_ordlist, sp, 0x1c23u);
    port_stack_leave(LF_FRAME);
    return 0;
}
