/* Routines of map/btlretr.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2fe9:0f66 battle_morale_check (())
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_morale_check)
{
    (void)args; (void)count;
    opeu_battle_morale_check(ds, link_btlretr);
    return 0;
}
