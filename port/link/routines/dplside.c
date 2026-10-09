/* Routines of map/dplside.c reached by routine identifier (address-keyed
 * words, port.h; see map/btl_link.h). frame = the routine's emulated
 * BP (entry SP - 2), sp = its entry SP, on the modeled DGROUP stack.
 *   2af1:0fa0 deploy_battle (())
 */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_deploy_battle)
{
    (void)args; (void)count;
    opeu_deploy_battle(ds, link_dplside);
    return 0;
}
