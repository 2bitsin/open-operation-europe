/* Routines of core/advmenu.c reached by identifier:
 *   1dd2:209a advice menu (staff action table DS:90e2, data/dgorders.c)
 *   1dd2:20fc turn_action_5 (turn-action far table, data/ctrltext.c)
 * No arguments; `sp` is the entry SP (far return address). */
#include "link.h"

PORT_ROUTINE(opeu_code_1dd2_209a)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_code_1dd2_209a(ds, link_advmenu, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}

PORT_ROUTINE(opeu_turn_action_5)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(PORT_FRAME);
    opeu_turn_action_5(ds, link_advmenu, sp);
    port_stack_leave(PORT_FRAME);
    return 0;
}
