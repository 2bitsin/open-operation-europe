/* Link: core/persmenu.c (1d82:0004..0500 personnel orders menu, turn
 * action 4). Services, from the host branch:
 *  0 request_format(buffer, format, args...)  0480:0b9a (cdecl)
 *  1 control_message(text, person)           1707:0672
 *  2 control_panel()                         1707:02e2 title_panel_begin
 *  3 daily_show(id, side, offset, segment)   1707:03e4
 *  4 pending_finish(value)                   0a8e:005c
 *  5 list_dialog(list, limit, flags) -> AX   0a8e:0bc0
 *  6 delay_dialog(text, last, person, menu) -> AX  1707:0726
 *  7 find_group_selected_unit(group) -> AX   145c:0c46
 *  8 unit_selection_eligible(unit) -> AX     145c:0fc4
 *  9 far action(segment, offset) of a DS:8f6c table entry. */
#include "b2_common.h"

uint16_t link_persmenu(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: b2_request_format(ds, args, count); break;
    case 1: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 2: opeu_title_panel_begin(ds, link_dialogs_host()); break;
    case 3:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 4: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 5: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 6: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 7: r = opeu_find_group_selected_unit(ds, ARG(0)); break;
    case 8: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 9: r = (uint16_t)port_call(ds, (uint32_t)ARG(0) * 16u + ARG(1), NULL, 0); break;
    default: port_unhandled("persmenu", id, count);
    }
    return r;
}
