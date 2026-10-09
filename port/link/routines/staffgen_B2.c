/* Routines of core/staffgen.c reached by identifier: 06db:0d28, the
 * selection-change callback of the Chief of Operations list (far pointer in
 * data/dgsetup.c). No arguments; result AL. */
#include "core/b2_common.h"

PORT_ROUTINE(opeu_code_06db_0d28)
{
    (void)args; (void)count;
    return opeu_code_06db_0d28(ds, link_staffgen);
}
