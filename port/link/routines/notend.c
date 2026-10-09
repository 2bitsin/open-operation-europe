/* Routines of core/notend.c reached by identifier: 3424:0082 is the far
 * presenter opeu_end_occupy passes to daily_show (called back through
 * DIALOGS' present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_end_occupy_present)
{
    (void)args; (void)count;
    opeu_end_occupy_present(ds, link_notend);
    return 0;
}
