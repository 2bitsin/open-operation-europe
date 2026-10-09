/* map/units routines reached by address from the battle modules and stockord:
 *   map/query.c    145c:011c find_unit_at (AL x, DL y) -> AX
 *                  145c:016a find_objective_at (AL x, DL y) -> AX
 *                  145c:01b8 objective_side (BX objective) -> AL
 *   units/resagg.c 145c:08fc objective_capacities (AX a, DX b, BX objective,
 *                  stack c): the three word results go through the pointers
 *   map/stepmap.c  145c:0d3e grid_step (AX y pointer, DL direction, BX x
 *                  pointer) -> AX
 *   map/objeleg.c  145c:101a objective_eligible (BX objective) -> AX
 *   map/mapunit.c  15c4:060e unit_flash (AL ticks, BX record); its host entry
 *                  takes its original BP (mapunit "Host frame convention").
 * Host entries that take host pointers get PORT_DS() of the DS offsets. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_find_unit_at)
{
    return opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_find_objective_at)
{
    return opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_objective_side)
{
    return opeu_objective_side(ds, ARG(0));
}

PORT_ROUTINE(opeu_objective_capacities)
{
    opeu_objective_capacities(ds, ARG(2), (uint16_t *)(void *)PORT_DS(ARG(0)),
                              (uint16_t *)(void *)PORT_DS(ARG(1)), (uint16_t *)(void *)PORT_DS(ARG(3)));
    return 0;
}

PORT_ROUTINE(opeu_grid_step)
{
    (void)ds;
    return opeu_grid_step(PORT_DS(ARG(2)), PORT_DS(ARG(0)), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_objective_eligible)
{
    return opeu_objective_eligible(ds, ARG(0));
}

PORT_ROUTINE(opeu_unit_flash)
{
    uint16_t sp = btl_sp_enter();
    opeu_unit_flash(ds, link_mapunit, BTL_BP(sp), ARG(1), (uint8_t)ARG(0));
    btl_sp_leave();
    return 0;
}
