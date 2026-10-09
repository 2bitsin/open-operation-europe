/* Routines of map/btldepty.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2fe9:01b6 battle_element_lost (BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_element_lost)
{
    opeu_battle_element_lost(ds, link_btldepty, ARG(0));
    return 0;
}
