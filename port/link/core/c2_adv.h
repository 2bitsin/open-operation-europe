/* The staff-advice services of the 1dd2 modules (advtact, advregt,
 * advenemy, advtide, advhints, advmenu). Ids from their host-branch
 * comments, callees from their DOS branches:
 *   0 request_format(dest,format,words...) 0480:0b9a
 *   1 control_message(text,person) 1707:0672 (void: result 0)
 *   2 delay_dialog(text,last,person,menu) 1707:0726 -> choice (advmenu)
 *   3 far call (offset,segment) of the DS:9060 / DS:90e2 action tables,
 *     AX returned (advmenu)
 * Conversion only. */
#ifndef PORT_LINK_CORE_C2_ADV_H
#define PORT_LINK_CORE_C2_ADV_H

#include "c2_common.h"

/* far_text parameter of the advice routines: far data segment 3ad9
 * (FADVICE), indexed by its offsets. */
#define C2_ADVICE_TEXT() ((const uint8_t *)port_far(0x3ad9u, 0))

static inline uint16_t c2_adv_service(const char *module, uint8_t *ds, uint16_t id,
                                      const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: c2_request_format(ds, args, count); break;                            /* 0480:0b9a */
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    default: port_unhandled(module, id, count);
    }
    return r;
}

#endif
