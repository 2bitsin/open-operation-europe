/* Routines of core/vwarms.c reached by identifier: far code pointers 1fe6:xxxx
 * in the DGSUPPLY view menus / DGORDERS staff table, called through a far
 * call (view VW_FAR_CALL, advice menu service 3) with no arguments. The
 * host entries take sp = entry SP (BP = sp-2). */
#include "core/b2_common.h"

/* 1fe6:0b02 */
PORT_ROUTINE(opeu_view_info_menu)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_info_menu(ds, link_vwarms, sp));
    return 0;
}

/* 1fe6:0ba6 */
PORT_ROUTINE(opeu_view_own_weapons)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_own_weapons(ds, link_vwarms, sp));
    return 0;
}

/* 1fe6:0be4 */
PORT_ROUTINE(opeu_view_enemy_weapons)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_enemy_weapons(ds, link_vwarms, sp));
    return 0;
}

/* 1fe6:0c28 */
PORT_ROUTINE(opeu_view_arms_menu)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_arms_menu(ds, link_vwarms, sp));
    return 0;
}

/* 1fe6:0c64 */
PORT_ROUTINE(opeu_view_victory)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_victory(ds, link_vwarms, sp));
    return 0;
}

/* 1fe6:0c90 -> AX */
PORT_ROUTINE(opeu_view_menu_loop)
{
    uint16_t r = 0;
    (void)args; (void)count;
    B2_SP_CALL(0x200u, r = opeu_view_menu_loop(ds, link_vwarms, sp));
    return r;
}
