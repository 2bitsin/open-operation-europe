/* Routines of map/btlmap.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2a96:0592 battle_map_setup (())
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_map_setup)
{
    (void)args; (void)count;
    opeu_battle_map_setup(ds, link_btlmap);
    return 0;
}
