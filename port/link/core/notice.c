/* Link: core/notice.c (33ad:0766 opeu_notice_begin). Services, from the
 * host branch and tests/validate_notice.py SERVICES:
 * 0 find_objective_at(x,y), 1 find_unit_at(x,y),
 * 2 unit_capacities(unit,result_a,result_b,result_c), 3 unit_side(unit),
 * 4 grid_step(x pointer,y pointer,direction), 5 find_element_unit(element),
 * 6 unit_selection_eligible(unit), 7 objective_eligible(objective),
 * 8 unit_group(unit), 9 objective_side(objective),
 * 10 request_format(destination,format,...) (cdecl),
 * 11 find_side_record(side,kind), 12 notice_confirm(text,person),
 * 13 pending_finish(value), 14 unit_special(unit), 15 unit_name(unit,buffer),
 * 16 string_copy(destination,source), 17 unit_message(unit,text),
 * 18 notice_objective_report(objective,text),
 * 19 daily_show(id,side,callback offset,callback segment).
 * Pointer arguments/results are DS offsets. */
#include "c1_common.h"

static void notice_set_word(uint8_t *ds, uint16_t at, uint16_t value)
{
    ds[at] = (uint8_t)value;
    ds[(uint16_t)(at + 1)] = (uint8_t)(value >> 8);
}

uint16_t link_notice(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;     /* 145c:016a */
    case 1: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;          /* 145c:011c */
    case 2: {                                                                             /* 145c:072c */
        uint16_t a, b, c;
        opeu_unit_capacities(ds, ARG(0), &a, &b, &c);
        notice_set_word(ds, ARG(1), a);
        notice_set_word(ds, ARG(2), b);
        notice_set_word(ds, ARG(3), c);
        break;
    }
    case 3: r = opeu_unit_side(ARG(0)); break;                                           /* 145c:006c */
    case 4: r = opeu_grid_step(ds + ARG(0), ds + ARG(1), (uint8_t)ARG(2)); break;        /* 145c:0d3e */
    case 5: r = opeu_find_element_unit(ds, ARG(0)); break;                               /* 145c:00d0 */
    case 6: r = opeu_unit_selection_eligible(ds, ARG(0)); break;                         /* 145c:0fc4 */
    case 7: r = opeu_objective_eligible(ds, ARG(0)); break;                              /* 145c:101a */
    case 8: r = opeu_unit_group(ARG(0)); break;                                          /* 145c:0086 */
    case 9: r = opeu_objective_side(ds, ARG(0)); break;                                  /* 145c:01b8 */
    case 10: c1_request_format(ds, args, count); break;                                  /* 0480:0b9a */
    case 11: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;    /* 0b73:02ee */
    case 12: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;  /* 1707:076c */
    case 13: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;             /* 0a8e:005c */
    case 14: r = opeu_unit_special(ds, ARG(0)); break;                                  /* 145c:04de */
    case 15: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;              /* 0b73:0188 */
    case 16: r = opeu_string_copy(ds, ARG(0), ARG(1)); break;                           /* 09cc:01d0 */
    case 17: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;          /* 1707:07ec */
    case 18: opeu_objective_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;     /* 1707:0890 */
    case 19:                                                                            /* 1707:03e4 */
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    default: port_unhandled("notice", id, count);
    }
    return r;
}
