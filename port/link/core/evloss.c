/* Link: core/evloss.c (108c:05d8..094a hourly unit-loss events). Services
 * from the host branch comment:
 *   0 unit_quantity(unit,category,count pointer)  1 category_7(code)
 *   2 unit_selection_eligible(unit)    3 unit_special(unit)
 *   4 unit_name(unit,buffer)           5 message_lookup(id)
 *   6 request_format(destination,format,arguments...) (cdecl)
 *   7 unit_message(unit,text)          8 1707:091a(unit,text)
 *   9 15c4:087e(x,y,side)              10 daily_show(id,side,offset,segment)
 *   11 music_command7()                12 unit_casualties(unit,a,b,c,d,e)
 *   13 sound_cue12()                   14 pending_finish(value) */
#include "c1_common.h"

uint16_t link_evloss(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_unit_quantity(ds, ARG(0), (uint8_t)ARG(1), PORT_DS(ARG(2))); break;   /* 145c:0596 */
    case 1: r = opeu_category_7((uint8_t)ARG(0)); break;                                  /* 0b73:0056 */
    case 2: r = opeu_unit_selection_eligible(ds, ARG(0)); break;                          /* 145c:0fc4 */
    case 3: r = opeu_unit_special(ds, ARG(0)); break;                                     /* 145c:04de */
    case 4: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;                 /* 0b73:0188 */
    case 5: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;            /* 312c:000c */
    case 6: c1_request_format(ds, args, count); break;                                    /* 0480:0b9a */
    case 7: opeu_unit_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;             /* 1707:07ec unit_message */
    case 8: opeu_unit_side_report(ds, link_dialogs_host(), ARG(0), ARG(1)); break;        /* 1707:091a */
    case 9:                                                                                /* 15c4:087e */
        frame = c1_frame_enter();
        opeu_location_flash(ds, link_mapunit, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 10: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                    /* 1707:03e4 */
    case 11: opeu_music_command7(ds, link_sound); break;                                  /* 0d65:01b0 */
    case 12:                                                                               /* 17c4:0b1c */
        frame = c1_frame_enter();
        opeu_unit_casualties(ds, link_casualty, frame, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        c1_frame_leave();
        break;
    case 13: opeu_sound_cue12(ds, link_soundcue); break;                                  /* 0d92:0068 */
    case 14: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break;               /* 0a8e:005c */
    default: port_unhandled("evloss", id, count);
    }
    return r;
}
