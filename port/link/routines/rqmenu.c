/* Routines of core/rqmenu.c reached by identifier: 22f9:0970
 * turn_action_7 (turn-action far table, data/ctrltext.c). No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_turn_action_7)
{
    (void)args; (void)count;
    opeu_turn_action_7(ds, link_rqmenu);
    return 0;
}
