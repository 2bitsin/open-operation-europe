/* Link: units/organize.c (20b3:1f4e turn action 6, reorganize a regiment).
 * Services from the host branch's list (callee by address):
 *  0 control_panel 1707:02e2 (title_panel_begin)  1 control_reset 1707:038e
 *  (screen_reset)  2 request_format 0480:0b9a  3 control_message 1707:0672
 *  4 control_confirm 1707:076c (confirm_message)  5 list_dialog 0a8e:0bc0
 *  6 unit_group_notice(flags,unit) 15c4:064a (unit_announce)
 *  7 copy_bytes 09cc:000a  8 daily_context 1389:000e  9 daily_show 1707:03e4
 * 10 unit_index 145c:0034  11 category_0_to_3 0b73:002e  12 grid_step
 * 145c:0d3e  13 find_objective_at 145c:016a  14 objective_side 145c:01b8
 * 15 release_side_elements 145c:14d2  16 control_position 15c4:13d8
 * 17 arrival_location 15c4:1408  18 list_length 20b3:0b24
 * 19 side_supported_units 20b3:0bd2  20 neighbor_units 20b3:0cae
 * 21 ops_chief_text 20b3:007a  22 ops_chief_wait 20b3:0106
 * 23 ops_chief_number 20b3:00da  24 supply_reset 20b3:0d5c
 * 25 tally_unit_stock 20b3:14e6  26 tally_units 20b3:156c
 * 27 alloc_screen 20b3:1642  28 alloc_take 20b3:1616
 * 29 alloc_groups_nonempty 20b3:1a36  30 alloc_type_panel 20b3:1982
 * 31 alloc_distribute 20b3:1b4e  32 alloc_choose_hq 20b3:1eae. */
#include "units_F.h"

/* Paragraph of code segment 20b3 in the original numbering that routine
 * identifiers use (allocdst stores it with the 10b8 dialog callback). */
#define LF_ORGANIZE_SEGMENT 0x20b3u

uint16_t link_organize(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (id) {
    case 0: opeu_title_panel_begin(ds, link_dialogs_host()); break;
    case 1: opeu_screen_reset(ds, link_dialogs_host()); break;
    case 2: r = lf_request_format(ds, args, count); break;
    case 3: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 4: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 5: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 6:
        /* The original pushes (flags, unit) to 15c4:064a, whose single
         * parameter is the unit: it receives the flags word. */
        sp = port_stack_enter(LF_FRAME);
        opeu_unit_announce(ds, link_mapunit, (uint16_t)(sp - 2u), ARG(0));
        port_stack_leave(LF_FRAME);
        break;
    case 7: opeu_copy_bytes(ds, ARG(0), ARG(1), ARG(2)); break;
    case 8: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    case 9:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1),
                        (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 10: r = opeu_unit_index(ARG(0)); break;
    case 11: r = opeu_category_0_to_3((uint8_t)ARG(0)); break;
    case 12: r = opeu_grid_step(PORT_DS(ARG(0)), PORT_DS(ARG(1)), (uint8_t)ARG(2)); break;
    case 13: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 14: r = opeu_objective_side(ds, ARG(0)); break;
    case 15: r = opeu_release_side_elements(ds, ARG(0), (uint8_t)ARG(1)); break;
    case 16: opeu_control_position(ds, ARG(0), ARG(1)); break;
    case 17:
        sp = port_stack_enter(LF_FRAME);
        opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2u), (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        port_stack_leave(LF_FRAME);
        break;
    case 18: r = opeu_list_length(ds, ARG(0)); break;
    case 19: r = opeu_side_supported_units(ds, (uint8_t)ARG(0), ARG(1)); break;
    case 20: r = opeu_neighbor_units(ds, ARG(0), ARG(1)); break;
    case 21: opeu_ops_chief_text(ds, link_chief, ARG(0)); break;
    case 22: r = opeu_ops_chief_wait(ds, link_chief, ARG(0)); break;
    case 23:
        /* buffer: the 30-byte prompt text of the callee's frame (BP-1e) */
        sp = port_stack_enter(LF_FRAME);
        r = opeu_ops_chief_number(ds, link_chief, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4),
                                  (uint16_t)(sp - 2u - 0x1eu));
        port_stack_leave(LF_FRAME);
        break;
    case 24: opeu_supply_reset(ds); break;
    case 25: r = opeu_tally_unit_stock(ds, link_tally, ARG(0), ARG(1)); break;
    case 26: r = opeu_tally_units(ds, link_tally, ARG(0), ARG(1)); break;
    case 27:
        /* buffer: the 22-byte nation text of the callee's frame (BP-1a) */
        sp = port_stack_enter(LF_FRAME);
        opeu_alloc_screen(ds, link_allocscr, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2),
                          (uint8_t)ARG(3), (uint16_t)(sp - 2u - 0x1au));
        port_stack_leave(LF_FRAME);
        break;
    case 28: opeu_alloc_take(ds, link_allocscr, ARG(0), (uint8_t)ARG(1), ARG(2), (uint8_t)ARG(3)); break;
    case 29: r = opeu_alloc_groups_nonempty(ds, link_allocpnl, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 30: opeu_alloc_type_panel(ds, link_allocpnl, (uint8_t)ARG(0), ARG(1)); break;
    case 31:
        opeu_alloc_distribute(ds, link_allocdst, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2), (uint8_t)ARG(3));
        break;
    case 32:
        /* text, name: the 100- and 32-byte buffers of the callee's frame */
        sp = port_stack_enter(LF_FRAME);
        opeu_alloc_choose_hq(ds, link_allocdst, (uint16_t)(sp - 2u - 0x64u),
                             (uint16_t)(sp - 2u - 0x84u), LF_ORGANIZE_SEGMENT);
        port_stack_leave(LF_FRAME);
        break;
    default: port_unhandled("organize", id, count);
    }
    return r;
}
