/* Routines of core/paradrop.c reached by identifier: 0f43:0d72 is the far
 * presenter the paratroop drop passes to daily_show. No arguments, no
 * result. */
#include "link.h"

PORT_ROUTINE(opeu_para_drop_present)
{
    (void)args; (void)count;
    opeu_para_drop_present(ds, link_paradrop);
    return 0;
}
