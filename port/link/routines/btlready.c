/* Routines of map/btlready.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2673:1488 battle_announce_ready (() -> AX) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_announce_ready)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    (void)args; (void)count;
    r = opeu_battle_announce_ready(ds, link_btlready, BTL_BP(sp));
    btl_sp_leave();
    return r;
}
