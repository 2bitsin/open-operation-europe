/* Routines of units/ordunit.c reached by identifier: the unit-row list
 * predicate 1c23:021c, stored as a far pointer into the organize list
 * (DS:5848+15) by opeu_turn_action_1 and called by the list dialog.
 * No arguments; the result is AL. */
#include "link.h"

PORT_ROUTINE(opeu_supply_unit_enabled)
{
    (void)args; (void)count;
    return opeu_supply_unit_enabled(ds);
}
