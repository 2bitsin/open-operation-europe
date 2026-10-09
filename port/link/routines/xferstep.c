/* Routines of units/xferstep.c reached by identifier: the transfer step
 * 31f4:05a8, passed as the far presenter of daily_show (xferstep service
 * 26, show(0x96,side,0x05a8,0x31f4)) and called back through the dialogs
 * link. No arguments. */
#include "link.h"

PORT_ROUTINE(opeu_transfer_step)
{
    (void)args; (void)count;
    opeu_transfer_step(ds, link_xferstep);
    return 0;
}
