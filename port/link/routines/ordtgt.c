/* Routines of map/ordtgt.c reached by identifier: the list predicates
 * ORDLIST stores as far pointers in its list records (1c23:0006 for the
 * group list, 1c23:0040 for the target list). No arguments; byte result
 * in AL (zero-extended). */
#include "link.h"

PORT_ROUTINE(opeu_group_target_enabled)
{
    (void)args; (void)count;
    return opeu_group_target_enabled(ds);
}

PORT_ROUTINE(opeu_menu_always)
{
    (void)ds; (void)args; (void)count;
    return opeu_menu_always();
}
