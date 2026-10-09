/* Routines of units/tally.c reached by identifier: 20b3:10b8, the choice
 * dialog callback stored as a far pointer into DS:5906+15 by
 * opeu_alloc_choose_hq (allocdst) and called by the list dialog.
 * No arguments; the result is AL. */
#include "link.h"

PORT_ROUTINE(opeu_tally_mark)
{
    (void)args; (void)count;
    return opeu_tally_mark(ds);
}
