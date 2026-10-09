/* Link: units/vwlist.c (1fe6:0004..026f view list builders and list-dialog
 * row checks). Services of the shared view numbering (vwlist.c host enum)
 * that this module raises:
 *   1 objective_side(objective) -> AL     145c:01b8
 *   2 objective_eligible(objective) -> AX 145c:101a
 *   3 group_selected_unit(group) -> AX    145c:0c46 find_group_selected_unit
 *   4 unit_eligible(unit) -> AX           145c:0fc4 unit_selection_eligible
 *   5 unit_side(unit) -> AL               145c:006c
 * The row checks are reached by identifier (far pointers 1fe6:xxxx stored in
 * the list dialogs): port/link/routines/vwlist.c. */
#include "units_F.h"

uint16_t link_vwlist(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 1: r = opeu_objective_side(ds, ARG(0)); break;
    case 2: r = opeu_objective_eligible(ds, ARG(0)); break;
    case 3: r = opeu_find_group_selected_unit(ds, ARG(0)); break;
    case 4: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 5: r = opeu_unit_side(ARG(0)); break;
    default: port_unhandled("vwlist", id, count);
    }
    return r;
}
