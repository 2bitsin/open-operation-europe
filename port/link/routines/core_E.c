/* core routines reached by address from the battle modules and stockord:
 *   core/people.c   0b73:02ee find_side_record (AL side, DL kind) -> AX
 *                   0b73:0334 find_group_record (AL side, DL group) -> AX
 *   core/deputy.c   0b73:048a initialize_deputy (BX person)
 *   core/context.c  1389:000e daily_context (AL side)
 *   core/recindex.c 145c:0048 unit_group_index (BX unit) -> AL
 *                   145c:006c unit_side (BX unit) -> AL
 *   core/objrank.c  190a:06d2 side_groups_retarget (AL side)
 *   core/emsmap.c   06ac:006a gfx_wait () (btldraw/btlsprt WAIT) */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_find_side_record)
{
    return opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_find_group_record)
{
    return opeu_find_group_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_initialize_deputy)
{
    opeu_initialize_deputy(ds, ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_daily_context)
{
    opeu_daily_context(ds, link_context, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_unit_group_index)
{
    (void)ds;
    return opeu_unit_group_index(ARG(0));
}

PORT_ROUTINE(opeu_unit_side)
{
    (void)ds;
    return opeu_unit_side(ARG(0));
}

PORT_ROUTINE(opeu_side_groups_retarget)
{
    opeu_side_groups_retarget(ds, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_gfx_wait)
{
    (void)args; (void)count;
    opeu_gfx_wait(ds, link_emsmap);
    return 0;
}
