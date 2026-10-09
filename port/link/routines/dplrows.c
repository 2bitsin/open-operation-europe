/* Routines of map/dplrows.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2af1:0002 deploy_true (() -> AL (list row predicate, far pointer DS:b54f))
 *   2af1:0006 deploy_draw_rows (AL column, DL first, BL row, stack last, flags (list draw callback, far pointer DS:b54b))
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_deploy_true)
{
    (void)args; (void)count;
    return opeu_deploy_true(ds, link_dplrows);
}

PORT_ROUTINE(opeu_deploy_draw_rows)
{
    opeu_deploy_draw_rows(ds, link_dplrows, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4));
    return 0;
}
