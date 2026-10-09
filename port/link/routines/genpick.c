/* Routines of units/genpick.c reached by identifier: the far code pointers
 * of the general-list windows in src/main/data/dgsetup.c:317..320, called by
 * the list UI (LISTUI events 19/20):
 *  06db:121e row draw (column, first, row, last, flags), bytes in prototype
 *            order;
 *  06db:0f3e choice callback (choice in AL) -> AL. */
#include "link.h"

PORT_ROUTINE(opeu_code_06db_121e)
{
    opeu_code_06db_121e(ds, link_genpick, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2),
                        (uint8_t)ARG(3), (uint8_t)ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_code_06db_0f3e)
{
    return opeu_code_06db_0f3e(ds, link_genpick, (uint8_t)ARG(0));
}
