/* Routines of core/dialogs.c reached by address from btlsetup and stockord:
 *   1707:02a2 info_panel_begin ()   1707:02e2 title_panel_begin ()
 *   1707:038e screen_reset ()       1707:03aa army_title (BX name)
 *   1707:0672 control_message (AX person, BX text)
 *   1707:06e4 control_choose (AX first, DX last, BX text, stack flag,
 *             person, menu) -> AX
 *   1707:079e confirm_ok (BX person) -> AX */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_info_panel_begin)
{
    (void)args; (void)count;
    opeu_info_panel_begin(ds, link_dialogs_host());
    return 0;
}

PORT_ROUTINE(opeu_title_panel_begin)
{
    (void)args; (void)count;
    opeu_title_panel_begin(ds, link_dialogs_host());
    return 0;
}

PORT_ROUTINE(opeu_screen_reset)
{
    (void)args; (void)count;
    opeu_screen_reset(ds, link_dialogs_host());
    return 0;
}

PORT_ROUTINE(opeu_army_title)
{
    opeu_army_title(ds, link_dialogs_host(), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_control_message)
{
    opeu_control_message(ds, link_dialogs_host(), ARG(1), ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_control_choose)
{
    return opeu_control_choose(ds, link_dialogs_host(), ARG(2), ARG(0), ARG(1), ARG(3), ARG(4), ARG(5));
}

PORT_ROUTINE(opeu_confirm_ok)
{
    return opeu_confirm_ok(ds, link_dialogs_host(), ARG(0));
}
