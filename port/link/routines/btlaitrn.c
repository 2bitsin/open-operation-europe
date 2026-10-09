/* Routines of map/btlaitrn.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2c49:15de battle_select_service (()) [sp]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_select_service)
{
    uint16_t sp = btl_sp_enter();
    (void)args; (void)count;
    opeu_battle_select_service(ds, link_btlaitrn, sp);
    btl_sp_leave();
    return 0;
}
