/* Routines of core/vwregt.c reached by identifier: far code pointers 1fe6:xxxx
 * in the DGSUPPLY view menus / DGORDERS staff table, called through a far
 * call (view VW_FAR_CALL, advice menu service 3) with no arguments. The
 * host entries take sp = entry SP (BP = sp-2). */
#include "core/b2_common.h"

/* 1fe6:09d6 */
PORT_ROUTINE(opeu_view_own_divisions)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_own_divisions(ds, link_vwregt, sp));
    return 0;
}

/* 1fe6:0a7a */
PORT_ROUTINE(opeu_view_enemy_units)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_enemy_units(ds, link_vwregt, sp));
    return 0;
}
