/* Link: map/blast.c (17c4:0744 location blast, 0860 blast erase, 0866 blast
 * pause). Events (host branch comment; callees from the DOS branch):
 *   0 side_for_status(side) -> low byte   0b73:04d0 opeu_side_for_status
 *   1 arrival_location(x,y,side)          15c4:1408 (MAPPOS)
 *   2 sound cue 5                         0d92:0030 opeu_sound_cue5
 *   3 wait(units)                         0a8e:004c opeu_delay_units
 *   4 blit_far(x,y,w,h,mode,seg,off)      0000:27ca opeu_blit_far
 *   5 sprite_compose (11 words)           0000:3e0a opeu_sprite_compose
 *   6 effect_finish                       0d65:02bc opeu_effect_finish
 *   7 map_erase(x,y)                      15c4:0f7c (MAPERASE)
 * 4 and 5 are the 15c4 kinds 9 and 8 (map_plan.c) word for word. */
#include "map_common.h"

uint16_t link_blast(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t frame, r = 0;
    switch (id) {
    case 0: r = opeu_side_for_status(ds, (uint8_t)ARG(0)); break;   /* 0b73:04d0 */
    case 1:                                                         /* 15c4:1408 */
        frame = map_frame_enter();
        opeu_arrival_location(ds, link_mappos, frame, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        map_frame_leave();
        break;
    case 2: opeu_sound_cue5(ds, link_soundcue); break;              /* 0d92:0030 */
    case 3: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break; /* 0a8e:004c */
    case 4: map_plan_service(ds, "blast", 9, args, count); break;   /* 0000:27ca */
    case 5: map_plan_service(ds, "blast", 8, args, count); break;   /* 0000:3e0a */
    case 6: opeu_effect_finish(ds, link_sound); break;              /* 0d65:02bc */
    case 7:                                                         /* 15c4:0f7c */
        frame = map_frame_enter();
        opeu_map_erase(ds, link_maperase, frame, (uint8_t)ARG(0), (uint8_t)ARG(1));
        map_frame_leave();
        break;
    default: port_unhandled("blast", id, count);
    }
    return r;
}
