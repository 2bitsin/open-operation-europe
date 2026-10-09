/* Link: core/daypres.c (0e36:0000 opeu_daily_present). Services:
 * request_wait=0, element_type_panel=1 (record), request_format=2 (text,
 * format, name, suffix; cdecl), find_side_record=3 (side, kind),
 * find_person_template=4 (mask, kind), control_message=5 (text, person). */
#include "c1_common.h"

uint16_t link_daypres(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: opeu_request_wait(ds, link_soundcue); break;    /* 0d92:0060 */
    case 1: opeu_element_type_panel(ds, link_dialogs_host(), ARG(0)); break;     /* 1707:0a22 */
    case 2: c1_request_format(ds, args, count); break;    /* 0480:0b9a */
    case 3: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;     /* 0b73:02ee */
    case 4: r = opeu_find_person_template(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 0b73:03d2 */
    case 5: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;      /* 1707:0672 */
    default: port_unhandled("daypres", id, count);
    }
    return r;
}
