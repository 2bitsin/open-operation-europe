/* Routines of core/evbomb.c reached by identifier: the air-raid event
 * (evreport service 5) and its far presenter passed to daily_show. No
 * arguments, no result. load = 0: original segment numbering. */
#include "link.h"

/* opeu_bomb_event keeps its buffers down to BP-0xce and runs the animation
 * with BP-0xd4 (pixel words at -2/-4 below that). */
#define BOMB_RESERVE 0x140u

PORT_ROUTINE(opeu_bomb_present)
{
    (void)args; (void)count;
    opeu_bomb_present(ds, link_evbomb, 0);
    return 0;
}

PORT_ROUTINE(opeu_bomb_event)
{
    uint16_t sp;
    (void)args; (void)count;
    sp = port_stack_enter(BOMB_RESERVE);
    opeu_bomb_event(ds, link_evbomb, 0, (uint16_t)(sp - 2));
    port_stack_leave(BOMB_RESERVE);
    return 0;
}
