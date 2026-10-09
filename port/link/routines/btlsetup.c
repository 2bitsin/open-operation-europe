/* Routines of map/btlsetup.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:1676 battle_tire_leaders (())
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_tire_leaders)
{
    (void)args; (void)count;
    opeu_battle_tire_leaders(ds, link_btlsetup);
    return 0;
}
