/* Routines of core/stockord.c reached by routine identifier:
 *   1d1e:01c2 turn_action_2 (): control action table DS:7fb4 (control1
 *             service 10, no arguments). Host entry: entry SP and the
 *             run-time paragraph of segment 1d1e (the list's far predicate
 *             pointer; the port keeps the original paragraph numbering).
 *   1d1e:004e stock_city_enabled () -> AL: the list-dialog row predicate
 *             far pointer that turn_action_2 stores at DS:5a78+15. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_turn_action_2)
{
    uint16_t sp = btl_sp_enter();
    (void)args; (void)count;
    opeu_turn_action_2(ds, link_stockord, sp, 0x1d1eu);
    btl_sp_leave();
    return 0;
}

PORT_ROUTINE(opeu_stock_city_enabled)
{
    uint16_t sp = btl_sp_enter();
    uint32_t r;
    (void)args; (void)count;
    r = opeu_stock_city_enabled(ds, link_stockord, sp);
    btl_sp_leave();
    return r;
}
