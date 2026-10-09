/* Routines of core/vwdiv.c reached by identifier: far code pointers 1fe6:xxxx
 * in the DGSUPPLY view menus / DGORDERS staff table, called through a far
 * call (view VW_FAR_CALL, advice menu service 3) with no arguments. The
 * host entries take sp = entry SP (BP = sp-2). */
#include "core/b2_common.h"

/* 1fe6:04fa */
PORT_ROUTINE(opeu_view_division_contact)
{
    (void)args; (void)count;
    B2_SP_CALL(0x200u, opeu_view_division_contact(ds, link_vwdiv, sp));
    return 0;
}
