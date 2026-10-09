/* Link: core/dispatch.c (1389:0ba8 opeu_process_selected_turn). Services
 * from the host branch's event list (event(ds,id,a0,a1,a2,a3)):
 * normalize=0, initialize=1, daily=2, half-day=3, hourly A/B/C=4..6,
 * pre-context=7, find side/group=8/9, active begin=10, label=11, side
 * selection=12, input A/B=13/14, selected begin=15, control one/other=16/17,
 * group selection=18, group action=19, final predicate/action=20/21.
 * Pointer arguments/results are DS offsets. */
#include "c1_common.h"

uint16_t link_dispatch(uint8_t *ds, uint16_t id, uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_normalize_turn_state(ds); break;                        /* 1360:000a */
    case 1: opeu_initialize_turn_side(ds, link_sideinit); break;         /* 0d9b:0934 */
    case 2: opeu_turn_daily(ds, link_daily); break;                      /* 0e36:010a */
    case 3: opeu_turn_half_day(ds, link_halfday); break;                 /* 0e47:00e0 */
    case 4: opeu_turn_hourly_a(ds, link_msntick); break;                 /* 0e8e:0b4a */
    case 5:                                                              /* 108c:155c */
        sp = port_stack_enter(PORT_FRAME);
        opeu_turn_hourly_b(ds, link_evreport, sp);
        port_stack_leave(PORT_FRAME);
        break;
    case 6: opeu_turn_hourly_c(ds, link_influenc); break;                /* 1270:0efc */
    case 7: opeu_turn_pre_context(ds, link_context); break;              /* 1389:011a */
    case 8: r = opeu_find_side_record(ds, (uint8_t)a0, (uint8_t)a1); break;   /* 0b73:02ee */
    case 9: r = opeu_find_group_record(ds, (uint8_t)a0, (uint8_t)a1); break;  /* 0b73:0334 */
    case 10: opeu_turn_active_begin(ds, link_dialogs_host()); break;     /* 1707:020c */
    case 11: {                                                           /* 0480:0bb8 turn_label (cdecl) */
        const uint16_t words[4] = { a0, a1, a2, a3 };
        c1_cursor_print(ds, words, 4);
        break;
    }
    case 12: r = opeu_find_side_selected_unit(ds, (uint8_t)a0); break;  /* 145c:0ca6 */
    case 13: r = (uint16_t)opeu_turn_input_a(ds, link_mouse); break;     /* 05a8:0158 */
    case 14: r = opeu_turn_input_b(); break;                             /* SYSLIB 0000:4066 */
    case 15: opeu_turn_selected_begin(ds); break;                        /* 190a:0724 */
    case 16: {                                                           /* 1389:0186 */
        /* scratch is the 50-byte string local: BP = entry SP - 2 and the
         * words a,b,c lie at scratch+54/+52/+50 below BP (scratch = BP-56);
         * the mode byte and the far temporary sit below scratch. */
        sp = port_stack_enter(PORT_FRAME);
        opeu_turn_control_one(ds, link_control1, link_context, (uint16_t)(sp - 2 - 56));
        port_stack_leave(PORT_FRAME);
        break;
    }
    case 17: opeu_turn_control_other(ds, link_control2, link_context); break;  /* 1389:09ec */
    case 18: r = opeu_find_group_selected_unit(ds, a0); break;          /* 145c:0c46 */
    case 19: opeu_turn_group_action(ds, link_grptask); break;            /* 1389:0b0e */
    case 20: r = opeu_turn_final_predicate(ds, link_grptask); break;     /* 1389:0b4c */
    case 21:                                                             /* 32dc:0006 */
        sp = port_stack_enter(PORT_FRAME);
        opeu_turn_final_action(ds, link_finalact, sp);
        port_stack_leave(PORT_FRAME);
        break;
    default: port_unhandled("dispatch", id, 4);
    }
    return r;
}
