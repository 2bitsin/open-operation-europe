/* Link: core/control1.c (1389:0186 opeu_turn_control_one, the selected
 * side's command menu and map inspection loop). Services, from the host
 * branch's event list: prepare=0, begin=1, position=2, location=3,
 * summary=4, active=5, format=6, menu=7, message=8, coordinates=9,
 * far action=10 (offset, segment of a DS:7fb4 opeu_control_actions entry).
 * Pointer arguments/results are DS offsets. */
#include "b2_common.h"

uint16_t link_control1(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0:                                                  /* 1dd2:2088 control_prepare */
        B2_SP_CALL(PORT_FRAME, opeu_control_prepare(ds, link_advmenu, sp));
        break;
    case 1: opeu_control_begin(ds, link_soundcue); break;    /* 0d92:0048 */
    case 2: opeu_control_position(ds, ARG(0), ARG(1)); break; /* 15c4:13d8 */
    case 3:                                                  /* 15c4:1408 arrival_location */
        /* mappos frame convention: the callee's BP = entry SP - 2. */
        B2_SP_CALL(PORT_FRAME, opeu_arrival_location(ds, link_mappos, (uint16_t)(sp - 2),
                                                     (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)));
        break;
    case 4:                                                  /* 0bc3:0042 army_summary */
        /* armysum scratch: name[32] at BP-0x50 of its 82-byte frame. */
        B2_SP_CALL(PORT_FRAME, opeu_army_summary(ds, link_armysum, (uint16_t)(sp - 2 - 0x50)));
        break;
    case 5: opeu_turn_active_begin(ds, link_dialogs_host()); break;  /* 1707:020c */
    case 6: b2_request_format(ds, args, count); break;               /* 0480:0b9a (dest, fmt, rec1) */
    case 7:                                                          /* 1707:06e4 control_choose */
        r = opeu_control_choose(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        break;
    case 8: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;  /* 1707:0672 */
    case 9:                                                  /* 1389:02a0 control_coordinates(x, y) */
        /* coords scratch: the original BP (entry SP - 2), 144-byte workspace below. */
        B2_SP_CALL(PORT_FRAME, r = opeu_control_coordinates(ds, link_coords, (uint16_t)(sp - 2), ARG(0), ARG(1)));
        break;
    case 10:                                                 /* far action (offset, segment) */
        r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), NULL, 0);
        break;
    default: port_unhandled("control1", id, count);
    }
    return r;
}
