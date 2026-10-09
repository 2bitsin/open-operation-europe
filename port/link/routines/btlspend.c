/* Routines of map/btlspend.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2db6:07ee battle_spend_fuel (AX before, BX element)
 *   2db6:0904 battle_spend_ammo (AX before, BX element)
 *   2db6:09b8 battle_spend_stores (AX before, BX element)
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_spend_fuel)
{
    opeu_battle_spend_fuel(ds, link_btlspend, ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_spend_ammo)
{
    opeu_battle_spend_ammo(ds, link_btlspend, ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_battle_spend_stores)
{
    opeu_battle_spend_stores(ds, link_btlspend, ARG(1), ARG(0));
    return 0;
}
