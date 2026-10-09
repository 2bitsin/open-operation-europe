/* Link: core/grptask.c (1389:0b0e group action, 1389:0b4c final
 * predicate). Services: group task service=0, unit task service=1. */
#include "link.h"

uint16_t link_grptask(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    (void)a0; (void)a1; (void)a2; (void)a3;
    switch (id) {
    case 0: opeu_group_task_service(ds); break;      /* 1983:04ca */
    case 1: opeu_unit_task_service(ds); break;       /* 19df:0db2 */
    default: port_unhandled("grptask", id, 4);
    }
    return 0;
}
