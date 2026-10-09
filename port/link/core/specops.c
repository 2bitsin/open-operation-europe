/* Link: core/specops.c (24d5:0000..0a82 Special Forces missions). Service
 * ids shared by SPECOPS and SPECFRC (specops.c host comment,
 * tests/validate_special_forces.py SERVICES):
 *   0 request_format(dest,format,args...)  1 control_message(text,person)
 *   2 confirm_message(text,person)->word   3 message_lookup(id)->DS offset
 *   4 army_print(format,args...)           5 pending_finish(value)
 *   6 daily_show(id,side,offset,segment)   7 find_objective_at(x,y)->objective
 *   8 delay_dialog(text,x,person,menu)     9 army_title(text)
 *  10 map_pick_location(x pointer,y pointer)->word
 *  11 objective_side(objective)->side     12 army_position(x,y)
 *  13 daily_context(side)                 14 far action(segment,offset)
 *  15 find_unit_at(x,y)->unit             16 unit_side(unit)->side
 *  17 unit_special(unit)->word            18 unit_name(unit,buffer)->buffer
 *  19 cell_influence_side(x,y)->side      20 find_person_template(mask,kind) */
#include "c1_common.h"

uint16_t link_specops(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: c1_request_format(ds, args, count); break;                                       /* 0480:0b9a */
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;            /* 1707:0672 */
    case 2: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:076c */
    case 3: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;               /* 312c:000c */
    case 4: c1_cursor_print(ds, args, count); break;                                         /* 0480:0bb8 army_print */
    case 5: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;                   /* 0a8e:005c */
    case 6: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                        /* 1707:03e4 */
    case 7: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;         /* 145c:016a */
    case 8: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break; /* 1707:0726 */
    case 9: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;                         /* 1707:03aa */
    case 10:                                                                                  /* 17c4:0274 */
        /* frame: the callee's BP; its 20-byte workspace lies below it. */
        frame = c1_frame_enter();
        r = opeu_map_pick_location(ds, link_mappick, frame, ARG(0), ARG(1));
        c1_frame_leave();
        break;
    case 11: r = opeu_objective_side(ds, ARG(0)); break;                                     /* 145c:01b8 */
    case 12: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break; /* 0480:0808 */
    case 13: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;                   /* 1389:000e */
    case 14: port_call(ds, (uint32_t)ARG(0) * 16u + ARG(1), NULL, 0); break;                 /* DS:adb2 far action (segment, offset) */
    case 15: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;             /* 145c:011c */
    case 16: r = opeu_unit_side(ARG(0)); break;                                              /* 145c:006c */
    case 17: r = opeu_unit_special(ds, ARG(0)); break;                                       /* 145c:04de */
    case 18: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;                   /* 0b73:0188 */
    case 19: r = opeu_cell_influence_side(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;      /* 145c:0f14 */
    case 20: r = opeu_find_person_template(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;     /* 0b73:03d2 */
    default: port_unhandled("specops", id, count);
    }
    return r;
}
