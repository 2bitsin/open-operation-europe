/* Link: map/objdamag.c (17c4:102a objective loss line, 17c4:10cc objective
 * damage report). Events (host branch comment; callees from the DOS branch):
 *   0 location_blast(x,y,side)              17c4:0744 (BLAST)
 *   1 request_format(buffer,format,...)     0480:0b9a (TEXTOUT, sprintf)
 *   2 objective_side_report(objective,text) 1707:09b2 (DIALOGS)
 *   3 element_panel(element)                1707:0b76 (DIALOGS)
 *   4 string_append(destination,source)     09cc:01f6
 *   5 blast_erase(x,y)                      17c4:0860 (BLAST)
 *   6 title_panel_begin()                   1707:02e2 (DIALOGS)
 *   7 objective_side(objective) -> byte     145c:01b8
 *   8 opposite_side(side) -> byte           0b73:04c8 */
#include "map_common.h"

uint16_t link_objdamag(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0:                                                         /* 17c4:0744 */
        frame = map_frame_enter();
        opeu_location_blast(ds, link_blast, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    case 1: map_request_format(ds, args, count); break;             /* 0480:0b9a */
    case 2: opeu_objective_side_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:09b2 */
    case 3: opeu_element_panel(ds, link_dialogs_host(), ARG(0)); break; /* 1707:0b76 */
    case 4: r = opeu_string_append(ds, ARG(0), ARG(1)); break;      /* 09cc:01f6 */
    case 5: opeu_blast_erase(ds, link_blast, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 17c4:0860 */
    case 6: opeu_title_panel_begin(ds, link_dialogs_host()); break; /* 1707:02e2 */
    case 7: r = opeu_objective_side(ds, ARG(0)); break;             /* 145c:01b8 */
    case 8: r = opeu_opposite_side((uint8_t)ARG(0)); break;         /* 0b73:04c8 */
    default: port_unhandled("objdamag", id, count);
    }
    return r;
}
