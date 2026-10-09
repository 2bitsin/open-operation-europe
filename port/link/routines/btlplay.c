/* Routines of map/btlplay.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2ec8:11ea code_2ec8_11ea (()) [sp]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_code_2ec8_11ea)
{
    uint16_t sp = btl_sp_enter();
    (void)args; (void)count;
    opeu_code_2ec8_11ea(ds, link_btlplay, sp);
    btl_sp_leave();
    return 0;
}
