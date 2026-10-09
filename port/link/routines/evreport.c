/* Routines of core/evreport.c reached by identifier: 108c:1302 is the far
 * presenter the unit reports pass to daily_show (called back through
 * DIALOGS' present service). No arguments, no result. */
#include "link.h"

PORT_ROUTINE(opeu_report_present)
{
    (void)args; (void)count;
    opeu_report_present(ds, link_evreport);
    return 0;
}
