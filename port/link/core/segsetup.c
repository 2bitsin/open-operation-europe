/* Link: core/segsetup.c (08ea:0000..0237 start-up memory and file set-up).
 * Events, from the host branch's comment (src/main/core/segsetup.c):
 *   0..8 the EMM services of core/emsmap.c (SEGSETUP passes its own event
 *        to the EMS routines)                       -> link_emsmap
 *   9  army_print(format)        0480:0bb8 opeu_cursor_print (cdecl frame)
 *   10 turn_input_b              0000:4066 opeu_turn_input_b
 *   11 quit_to_dos               2597:0b8c opeu_quit_to_dos (SETTINGS)
 *   12 dos_alloc(paragraphs)     0000:00f3 opeu_dos_alloc
 *   13 open_resource(name)       0977:000e opeu_open_resource (RESOPEN)
 *   14 install_dos_handlers      0000:402e koei_install_dos_handlers
 *   15 vga 0000:2fea             koei_2fea
 *   16 vga_identity_palette      0000:23c4 koei_vga_identity_palette
 *   17 vga_clear                 0000:2aac opeu_vga_clear
 *   18 palette_title             0a04:0884 opeu_palette_title_screen (UIPRIM)
 *   19 sound_initialize          0d65:0002 opeu_sound_initialize (SOUND) */
#include "a_util.h"

uint16_t link_segsetup(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    switch (id) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8:
        return link_emsmap(ds, id, args, count);
    case 9: a_cursor_print(ds, args, 1); break;
    case 10: return opeu_turn_input_b();
    case 11: opeu_quit_to_dos(ds, link_settings); break;
    case 12: return opeu_dos_alloc(ARG(0));
    case 13: return (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0));
    case 14: koei_install_dos_handlers(); break;
    case 15: koei_2fea(); break;
    case 16: koei_vga_identity_palette(); break;
    case 17: opeu_vga_clear(); break;
    case 18: opeu_palette_title_screen(ds, link_uiprim_host()); break;
    case 19: opeu_sound_initialize(ds, link_sound); break;
    default: port_unhandled("segsetup", id, count);
    }
    return 0;
}
