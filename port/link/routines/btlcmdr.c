/* Routines of map/btlcmdr.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   29f5:0008 battle_restore_tile (AL column, DL row)
 *   29f5:0030 battle_info_clear (())
 *   29f5:006c battle_panel_begin (())
 *   29f5:00b2 battle_panel_side1 (())
 *   29f5:00f8 battle_panel_side0 (())
 *   29f5:014c battle_name_clear_left (())
 *   29f5:019c battle_name_clear_right (())
 *   29f5:01fa battle_commander_panel (AL side, DX refresh) [frame]
 *   29f5:0334 battle_commander_wait (AL side, DL value, BX text) [frame]
 *   29f5:0362 battle_commander_message (AL side, DL value, BX text) [frame]
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_battle_restore_tile)
{
    opeu_battle_restore_tile(ds, link_btlcmdr, (uint8_t)ARG(0), (uint8_t)ARG(1));
    return 0;
}

PORT_ROUTINE(opeu_battle_info_clear)
{
    (void)args; (void)count;
    opeu_battle_info_clear(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_panel_begin)
{
    (void)args; (void)count;
    opeu_battle_panel_begin(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_panel_side1)
{
    (void)args; (void)count;
    opeu_battle_panel_side1(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_panel_side0)
{
    (void)args; (void)count;
    opeu_battle_panel_side0(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_name_clear_left)
{
    (void)args; (void)count;
    opeu_battle_name_clear_left(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_name_clear_right)
{
    (void)args; (void)count;
    opeu_battle_name_clear_right(ds, link_btlcmdr);
    return 0;
}

PORT_ROUTINE(opeu_battle_commander_panel)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_commander_panel(ds, link_btlcmdr, (uint8_t)ARG(0), ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_commander_wait)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_commander_wait(ds, link_btlcmdr, ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_battle_commander_message)
{
    uint16_t sp = btl_sp_enter();
    opeu_battle_commander_message(ds, link_btlcmdr, ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1), BTL_BP(sp));
    btl_sp_leave();
    return 0;
}
