/* Link: core/paradrop.c (0f43:0c22..0f46 paratroop drop). Services from the
 * host branch comment (tests/validate_para_drop.py SERVICES):
 *   0 unit_group(unit)            1 para_cell_ok(x,y)      2 air_report(days,text)
 *   3 find_side_record(side,kind) 4 find_person_template(mask,kind)
 *   5 opposite_side(side)         6 request_message(text,person)
 *   7 0d92:0028()                 8 side_for_status(side)  9 map_center(x,y,side)
 *   10 view_pixel(&x,&y)          11 daily_show(id,side,offset,segment)
 *   12 music_command7()           13 para_unit_create(side,order)
 *   14 para_land(unit,losses)     15 arrival_prepare(unit) 16 input_delay(ticks)
 *   17 air_sprite_draw(x,y,side)  18 air_sprite_restore(x,y)
 *   19 air_damage_roll(x,y,n)     20 air_destroyed_message(text)
 *   21 officer_gain(side,kind)
 * AIRSHOW/AIRSTRK `load` parameters are 0 (original segment numbering);
 * their `bp` parameters are the callee's BP (entry SP - 2). */
#include "c1_common.h"

uint16_t link_paradrop(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_unit_group(ARG(0)); break;                                              /* 145c:0086 */
    case 1: r = opeu_para_cell_ok(ds, link_paraunit, (uint8_t)ARG(0), (uint8_t)ARG(1)); break; /* 0f43:089a */
    case 2:                                                                                   /* 0f43:01da */
        /* air_report(what, wait): the text is BX, the days byte AL. */
        frame = c1_frame_enter();
        r = opeu_air_report(ds, link_airshow, frame, ARG(1), (uint8_t)ARG(0));
        c1_frame_leave();
        break;
    case 3: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;          /* 0b73:02ee */
    case 4: r = opeu_find_person_template(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;      /* 0b73:03d2 */
    case 5: r = opeu_opposite_side((uint8_t)ARG(0)); break;                                  /* 0b73:04c8 */
    case 6: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;            /* 1707:0672 request_message */
    case 7: opeu_sound_cue4(ds, link_soundcue); break;                                       /* 0d92:0028 */
    case 8: r = opeu_side_for_status(ds, (uint8_t)ARG(0)); break;                            /* 0b73:04d0 */
    case 9:                                                                                   /* 15c4:13f6 */
        frame = c1_frame_enter();
        opeu_map_center(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 10: opeu_view_pixel(ds, ARG(0), ARG(1)); break;                                     /* 15c4:055c */
    case 11: c1_daily_show(ds, ARG(0), ARG(1), ARG(2), ARG(3)); break;                       /* 1707:03e4 */
    case 12: opeu_music_command7(ds, link_sound); break;                                     /* 0d65:01b0 */
    case 13: r = opeu_para_unit_create(ds, link_paraunit, (uint8_t)ARG(0), PORT_DS(ARG(1))); break; /* 0f43:0722 */
    case 14: r = opeu_para_land(ds, link_paraunit, PORT_DS(ARG(0)), ARG(1)); break;          /* 0f43:0954 */
    case 15:                                                                                  /* 15c4:0fd0 */
        frame = c1_frame_enter();
        opeu_arrival_prepare(ds, link_maperase, frame, ARG(0));
        c1_frame_leave();
        break;
    case 16: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;                              /* 0a8e:000c */
    case 17:                                                                                  /* 0f43:00e0 */
        frame = c1_frame_enter();
        opeu_air_sprite_draw(ds, link_airshow, 0, frame, ARG(0), ARG(1), (uint8_t)ARG(2));
        c1_frame_leave();
        break;
    case 18: opeu_air_sprite_restore(ds, link_airshow, 0, ARG(0), ARG(1)); break;            /* 0f43:01ac */
    case 19: r = opeu_air_damage_roll(ds, link_airstrk, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break; /* 0f43:028a */
    case 20:                                                                                  /* 0f43:0362 */
        frame = c1_frame_enter();
        opeu_air_destroyed_message(ds, link_airstrk, frame, ARG(0));
        c1_frame_leave();
        break;
    case 21: opeu_officer_gain(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;                 /* 0e8e:002c */
    default: port_unhandled("paradrop", id, count);
    }
    return r;
}
