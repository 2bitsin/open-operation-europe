/* Routines of units/allocdst.c reached by identifier: 20b3:1f46, the
 * presentation callback that opeu_turn_action_6 (organize) passes as a far
 * code pointer to opeu_daily_show; the dialogs link reaches it through
 * port_call (dialogs event 47 present(offset,segment)). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_alloc_finish)
{
    (void)args; (void)count;
    opeu_alloc_finish(ds, link_allocdst);
    return 0;
}
