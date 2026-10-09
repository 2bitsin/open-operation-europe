/* Routines of map/btlpath.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2a4a:0088 battle_path_step (AX y pointer, BX x pointer) [frame]
 *   2a4a:0110 battle_path_flood (AL x, DL y, BL to_x, stack to_y, element, mode) [frame]
 *   2a4a:03ca battle_spot_enemies (BX element) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_path_step)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_path_step(ds, link_btlpath, ARG(1), ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_path_flood)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    r = opeu_battle_path_flood(ds, link_btlpath, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), ARG(4), (uint8_t)ARG(5), BTL_BP(sp));
    btl_sp_leave();
    return r;
}

PORT_ROUTINE(opeu_battle_spot_enemies)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_spot_enemies(ds, link_btlpath, ARG(0), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
