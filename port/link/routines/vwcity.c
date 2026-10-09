/* Routines of core/vwcity.c reached by identifier: far code pointers 1fe6:xxxx
 * in the DGSUPPLY view menus / DGORDERS staff table, called through a far
 * call (view VW_FAR_CALL, advice menu service 3) with no arguments. The
 * host entries take sp = entry SP (BP = sp-2). */
#include "core/b2_common.h"

/* 1fe6:082e */
PORT_ROUTINE(opeu_view_own_cities)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_own_cities(ds, link_vwcity, sp));
    return 0;
}

/* 1fe6:085e */
PORT_ROUTINE(opeu_view_enemy_cities)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_enemy_cities(ds, link_vwcity, sp));
    return 0;
}

/* 1fe6:0894 */
PORT_ROUTINE(opeu_view_cities_menu)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_cities_menu(ds, link_vwcity, sp));
    return 0;
}
