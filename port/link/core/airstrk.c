/* Link: core/airstrk.c (0f43:028a..0722, air strike). Services, from the
 * host branch's event list (airstrk.c:17):
 *   0 officer_rating(side,kind)  0e8e:0000   1 outside_5_to_19(hour)     0b73:0178
 *   2 find_unit_at(x,y)          145c:011c   3 unit_side(unit)           145c:006c
 *   4 find_objective_at(x,y)     145c:016a   5 objective_side(objective) 145c:01b8
 *   6 request_format(dest,format,...) 0480:0b9a
 *   7 find_side_record(side,kind) 0b73:02ee  8 request_message(text,person) 1707:0672
 *   9 air_report(count,text)     0f43:01da  10 find_person_template(mask,kind) 0b73:03d2
 *  11 opposite_side(side)        0b73:04c8  12 sound cue                 0d92:0028
 *  13 map_code(x,y)              145c:0dc0  14 isqrt(value)              0000:3fce
 *  15 unit_casualties(unit,a..e) 17c4:0b1c  16 objective_damage(o,a..f)  17c4:10cc
 *  17 set_turn_context(value)    0d65:00fe  18 side_for_status(side)     0b73:04d0
 *  19 map_center(x,y,side)       15c4:13f6  20 view_pixel(x addr,y addr) 15c4:055c
 *  21 air_sprite_draw(x,y,side)  0f43:00e0  22 daily_show(id,side,off,seg) 1707:03e4
 *  23 music_command7()           0d65:01b0  24 delay_ticks(n)            0a8e:000c
 *  25 air_sprite_restore(x,y)    0f43:01ac  26 officer_gain(side,kind)   0e8e:002c
 *  27 finish_side_initialization(v) 0d65:0032 28 music_stop()            0d65:0060
 *  29 turn_pre_context()         1389:011a
 * The sprite routines take the load segment: 0 (original numbering). */
#include "c2_air.h"

uint16_t link_airstrk(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, bp;
    switch (id) {
    case 0: r = opeu_officer_rating(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 1: r = opeu_code_outside_5_to_19((uint8_t)ARG(0)); break;
    case 2: r = opeu_find_unit_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 3: r = opeu_unit_side(ARG(0)); break;
    case 4: r = opeu_find_objective_at(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 5: r = opeu_objective_side(ds, ARG(0)); break;
    case 6: c2_request_format(ds, args, count); break;
    case 7: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 8: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 9:                                                  /* (count, text) */
        bp = c2_air_bp_enter();
        r = opeu_air_report(ds, link_airshow, bp, ARG(1), (uint8_t)ARG(0));
        c2_air_leave();
        break;
    case 10: r = opeu_find_person_template(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 11: r = opeu_opposite_side((uint8_t)ARG(0)); break;
    case 12: opeu_sound_cue4(ds, link_soundcue); break;
    case 13: r = opeu_map_code(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 14: r = opeu_isqrt(ARG(0)); break;                  /* _cdecl opeu_isqrt(uint16_t) */
    case 15:
        bp = c2_air_bp_enter();
        opeu_unit_casualties(ds, link_casualty, bp, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5));
        c2_air_leave();
        break;
    case 16:
        bp = c2_air_bp_enter();
        opeu_objective_damage(ds, link_objdamag, bp, ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5), ARG(6));
        c2_air_leave();
        break;
    case 17: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    case 18: r = opeu_side_for_status(ds, (uint8_t)ARG(0)); break;
    case 19:
        bp = c2_frame_enter();
        opeu_map_center(ds, link_mappos, bp, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        c2_frame_leave();
        break;
    case 20: opeu_view_pixel(ds, ARG(0), ARG(1)); break;
    case 21:
        bp = c2_air_bp_enter();
        opeu_air_sprite_draw(ds, link_airshow, 0, bp, ARG(0), ARG(1), (uint8_t)ARG(2));
        c2_air_leave();
        break;
    case 22:
        opeu_daily_show(ds, link_dialogs_host(), ARG(0), (uint8_t)ARG(1), (uint32_t)ARG(3) << 16 | ARG(2));
        break;
    case 23: opeu_music_command7(ds, link_sound); break;
    case 24: opeu_delay_ticks(ds, link_waitkey, ARG(0)); break;
    case 25: opeu_air_sprite_restore(ds, link_airshow, 0, ARG(0), ARG(1)); break;
    case 26: opeu_officer_gain(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 27: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    case 28: opeu_music_stop(ds, link_sound); break;
    case 29: opeu_turn_pre_context(ds, link_context); break;
    default: port_unhandled("airstrk", id, count);
    }
    return r;
}
