/* Routines of map/btladv.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:0d2e battle_element_step (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_element_step)
{
    opeu_battle_element_step(ds, link_btladv, ARG(0));
    return 0;
}
