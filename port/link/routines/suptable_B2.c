/* Routines of core/suptable.c reached by identifier (segment 20b3):
 *  0d40 supply_unit_eligible  select/eligibility callback stored by
 *                             units/organize.c (list +15/+17) -> AL
 *  0dfa supply_true, 0ed8 supply_return_slot
 *                             select callbacks in data/dgsupply.c -> AL
 *                             (LISTUI event 20: the row in AL, unused)
 *  0dfe supply_draw_types, 0fac supply_draw_slots
 *                             row-draw callbacks in data/dgsupply.c (LISTUI
 *                             event 19): (column, first, row, last, flags),
 *                             bytes zero-extended, in prototype order.
 * The draw entries keep their name buffers at DS `scratch` (22 bytes for
 * the type view, 32 + 22 for the slot view), placed in a modeled frame. */
#include "core/b2_common.h"

#define SUPPLY_SCRATCH(sp) ((uint16_t)((sp) - 2 - 0x40))

PORT_ROUTINE(opeu_supply_unit_eligible)
{
    (void)args; (void)count;
    return opeu_supply_unit_eligible(ds);
}

PORT_ROUTINE(opeu_supply_true)
{
    (void)ds; (void)args; (void)count;
    return opeu_supply_true();
}

PORT_ROUTINE(opeu_supply_return_slot)
{
    (void)args; (void)count;
    return opeu_supply_return_slot(ds, link_suptable);
}

PORT_ROUTINE(opeu_supply_draw_types)
{
    B2_SP_CALL(PORT_FRAME, opeu_supply_draw_types(ds, link_suptable, SUPPLY_SCRATCH(sp), (uint8_t)ARG(0),
                                                  (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4)));
    return 0;
}

PORT_ROUTINE(opeu_supply_draw_slots)
{
    B2_SP_CALL(PORT_FRAME, opeu_supply_draw_slots(ds, link_suptable, SUPPLY_SCRATCH(sp), (uint8_t)ARG(0),
                                                  (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3), (uint8_t)ARG(4)));
    return 0;
}
