/* Link: core/startup.c (06db:1e08..20f0 new-game start-up service).
 * Events, from the host branch's comment (src/main/core/startup.c):
 *   0 BIOS INT 1Ah AH=0 -> DX (low tick word)   port_clock_ticks_low
 *   1 08ea:01ea opeu_app_prepare (SEGSETUP; entry SP)
 *   2 06db:1700 opeu_new_game_setup -> flag word (SCENSEL; entry SP)
 *   3 VGA clear 0000:2aac opeu_vga_clear
 *   4 0d65:0032 opeu_finish_side_initialization(value) (SOUND) */
#include "link.h"

/* app_prepare: 0Eh-byte frame, alloc_segments at sp-16h, the EMM detection
 * below it keeps 106h bytes of locals. */
#define PREPARE_FRAME 0x200u
/* new_game_setup: buffer at BP-648h plus its callees' frames. */
#define NEW_GAME_FRAME 0x900u

uint16_t link_startup(uint8_t *ds, uint16_t service, const uint16_t *args, uint16_t count)
{
    uint16_t sp, r = 0;
    switch (service) {
    case 0: r = port_clock_ticks_low(); break;
    case 1:
        sp = port_stack_enter(PREPARE_FRAME);
        opeu_app_prepare(ds, link_segsetup, sp);
        port_stack_leave(PREPARE_FRAME);
        break;
    case 2:
        sp = port_stack_enter(NEW_GAME_FRAME);
        r = (uint16_t)opeu_new_game_setup(ds, link_scensel, sp);
        port_stack_leave(NEW_GAME_FRAME);
        break;
    case 3: opeu_vga_clear(); break;
    case 4: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    default: port_unhandled("startup", service, count);
    }
    return r;
}
