/* Link: map/ordtgt.c (1c23:0006 group-row predicate, 1c23:0040 always,
 * 1c23:0044 set a group's order target). Events used by ORDTGT (host branch
 * comment; the numbering is shared with ORDUNIT/ORDLIST, whose own ids 5..8
 * ORDTGT never raises):
 *   0 request_format(dest,fmt,args...)   0480:0b9a (TEXTOUT, sprintf)
 *   1 control_message(text,person)       1707:0672 (DIALOGS)
 *   2 confirm_message(text,person) -> word  1707:076c (DIALOGS)
 *   3 arrival_location(x,y,side)         15c4:1408 (MAPPOS)
 *   4 pick_location(x_address,y_address) -> word  17c4:0274 (MAPPICK) */
#include "map_common.h"

uint16_t link_ordtgt(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: map_request_format(ds, args, count); break;             /* 0480:0b9a */
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:0672 */
    case 2: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break; /* 1707:076c */
    case 3:                                                         /* 15c4:1408 */
        frame = map_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    case 4:                                                         /* 17c4:0274 */
        frame = map_frame_enter();
        r = opeu_map_pick_location(ds, link_mappick, frame, ARG(0), ARG(1));
        map_frame_leave();
        break;
    default: port_unhandled("ordtgt", id, count);
    }
    return r;
}
