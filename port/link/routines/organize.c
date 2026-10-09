/* Routines of units/organize.c reached by identifier: 20b3:1f4e, turn
 * action 6 (reorganize a regiment), an entry of the control action table
 * DS:7fb4 (control1 service 10). No arguments. The host entry takes the
 * original BP (entry SP - 2; locals reach BP-0xa8) and the paragraph of
 * code segment 20b3 (stored in the far predicate 0d40 and daily_show's
 * presenter 1f46). */
#include "../units/units_F.h"

#define LF_ORGANIZE_RESERVE 0x300u

PORT_ROUTINE(opeu_turn_action_6)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(LF_ORGANIZE_RESERVE);
    opeu_turn_action_6(ds, link_organize, (uint16_t)(sp - 2u), 0x20b3u);
    port_stack_leave(LF_ORGANIZE_RESERVE);
    return 0;
}
