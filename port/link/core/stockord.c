/* Link: core/stockord.c (1d1e, the city stockpile order). Its host branch
 * names every callee by linear routine address with the arguments in source
 * order (stack arguments in prototype order); the routine handlers take the
 * register words in AX, DX, BX order (port.h), so the services with more
 * than one register argument are reordered here:
 *   14ebc objective_capacities (objective, a, b, c) -> (a, b, objective, c)
 *   1624e unit_flash (record, ticks)                -> (ticks, record)
 *   176e2 control_message (text, person)            -> (person, text)
 *   17754 control_choose (text, first, last, flag, person, menu)
 *                                                   -> (first, last, text, ...)
 *   0b4a0 list_dialog (window, limit, flags)        -> (limit, flags, window)
 * Every other service has one register word or is cdecl and forwards as is
 * (validate_stock_order.py token lists). */
#include "map/btl_link.h"

uint16_t link_stockord(uint8_t *ds, uint32_t service, const uint16_t *args, unsigned int count)
{
    static const unsigned char capacities[] = {1, 2, 0, 3};
    static const unsigned char swap[] = {1, 0};
    static const unsigned char choose[] = {1, 2, 0, 3, 4, 5};
    static const unsigned char list_dialog[] = {1, 2, 0};
    switch (service) {
    case 0x14ebcu: return btl_reorder(ds, service, args, count, capacities, 4);
    case 0x1624eu:
    case 0x176e2u: return btl_reorder(ds, service, args, count, swap, 2);
    case 0x17754u: return btl_reorder(ds, service, args, count, choose, 6);
    case 0x0b4a0u: return btl_reorder(ds, service, args, count, list_dialog, 3);
    default: return port_service(ds, service, args, count);
    }
}
