/* Routines of core/vwpeople.c reached by identifier: far code pointers 1fe6:xxxx
 * in the DGSUPPLY view menus / DGORDERS staff table, called through a far
 * call (view VW_FAR_CALL, advice menu service 3) with no arguments. The
 * host entries take sp = entry SP (BP = sp-2). */
#include "core/b2_common.h"

/* 1fe6:0656 */
PORT_ROUTINE(opeu_view_own_generals)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_own_generals(ds, link_vwpeople, sp));
    return 0;
}

/* 1fe6:06ae */
PORT_ROUTINE(opeu_view_enemy_generals)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_enemy_generals(ds, link_vwpeople, sp));
    return 0;
}

/* 1fe6:0724 */
PORT_ROUTINE(opeu_view_generals_menu)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_generals_menu(ds, link_vwpeople, sp));
    return 0;
}
