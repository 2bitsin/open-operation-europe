/* Routines of map/btlrpanl.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2db6:0004 battle_wiped_notice (BX element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_wiped_notice)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_wiped_notice(ds, link_btlrpanl, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
