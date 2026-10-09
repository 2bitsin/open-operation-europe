/* Routines of map/movespot.c reached by identifier: 32e6:05a0
 * move_mines_finish is the far presenter MOVEMINE passes to daily_show
 * (called back through DIALOGS' present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_move_mines_finish)
{
    (void)args; (void)count;
    opeu_move_mines_finish(ds, link_movespot);
    return 0;
}
