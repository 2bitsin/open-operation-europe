/* Routines of core/notice.c reached by identifier: 33ad:05fe is the far
 * presenter opeu_notice_attack passes to daily_show (called back through
 * DIALOGS' present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_notice_view_finish)
{
    (void)args; (void)count;
    opeu_notice_view_finish(ds, link_notice);
    return 0;
}
