/* Routines of units/xferunit.c reached by identifier: the supply-objective
 * and supply-unit order handlers 31f4:0ca6 / 31f4:0d06 of the DGBATTLE
 * far-code order table (DS:c0c8, finalact service 9). No arguments; they
 * return AX. Their host entries keep their frame words at the module's
 * fixed scratch offsets (XFER_SCRATCH..), so they take no sp. */
#include "link.h"

PORT_ROUTINE(opeu_order_supply_objective)
{
    (void)args; (void)count;
    return opeu_order_supply_objective(ds, link_xferunit);
}

PORT_ROUTINE(opeu_order_supply_unit)
{
    (void)args; (void)count;
    return opeu_order_supply_unit(ds, link_xferunit);
}
