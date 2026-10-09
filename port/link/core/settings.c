/* Link: core/settings.c (2597:000a..0dc7 the Settings command). Events,
 * from the host branch's comment, resolved by address (symbols.json and
 * the EVENTS table of tests/validate_settings.py):
 *   0 print(format,...) 0480:0bb8 opeu_army_print (host opeu_cursor_print)
 *   1 format(dest,format,value) 0480:0b9a opeu_request_format
 *   2 dialog(text,x,person,menu) 1707:0726 opeu_delay_dialog
 *   3 number(text,x,person,0) 1707:0740 opeu_control_number (host opeu_choose_dialog)
 *   4 message(text,person) 1707:0672 opeu_control_message
 *   5 ask(text,person) 1707:076c opeu_control_ask (host opeu_confirm_message)
 *   6 location(x,y,side) 15c4:1408 opeu_arrival_location
 *   7 music stop 0d65:0060   8 pre-context 1389:011a opeu_turn_pre_context
 *   9 screen reset 1707:038e  10 attribute(v) 0480:0838 (host opeu_text_paper_set)
 *  11 disk reset 0977:0038   12 framed(x,y,text) 0a04:0788 (host opeu_title_box)
 *  13 ui text 0480:0484      14 wait(t) 0a8e:004c opeu_delay_units
 *  15 open(name) 0977:000e   16 menu(menu,first,count) 0a8e:0bc0 opeu_list_dialog
 *  17 outline 0a04:065a      18 position 0480:0808   19 title 1707:03aa opeu_army_title
 *  20 yes/no(t) 0541:0664 (host opeu_prompt_yes_no)  21 active begin 1707:020c
 *  22 title palette 0a04:0884 (@opeu_palette_title, host opeu_palette_title_screen)
 *  23 default palette 0a04:0860 (host opeu_palette_daylight)
 *  24 finish side(v) 0d65:0032   25 turn context(v) 0d65:00fe
 *  26 free segments 090d:0544  27 memory release 06ac:0094
 *  28 close files 090d:0682    29 application entry 0977:03dc
 *  30 create 0000:00b2  31 exists 0000:00c5  32 write 0000:006e  33 read 0000:0043
 *  34 far read 0000:0057  35 close 0000:0032  36 seek 0000:009b  37 box 0000:2ad2
 *  38 VGA copy 0000:301f  39 bitmap 0000:2a96 (opeu_bitmap_draw = opeu_gfx_blit)
 *  40 FM wait 0000:4052  41 FM busy 0000:405f  42 sound open(0,0) 0000:4074
 *  43 FM no-op 0000:4048  44 text mode 0000:300c  45 exit(code) 0000:4259
 *  46 VGA clear 0000:2aac
 *  47 far action(offset,segment) of a table entry outside this module -> port_call.
 * The entry routine 2597:0d72 opeu_turn_action_10 is reached by identifier
 * (turn-action far table): port/link/routines/settings.c. */
#include "b1_util.h"

#define KEY_SCRATCH 16u   /* keyboard.c: 16-byte caller scratch area */

uint16_t link_settings(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, f;
    switch (id) {
    case 0: b1_cursor_print(ds, args, count); break;
    case 1: b1_request_format(ds, args, count); break;
    case 2: r = opeu_delay_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 3: r = (uint8_t)opeu_choose_dialog(ds, link_dialogs_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 4: opeu_control_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 5: r = opeu_confirm_message(ds, link_dialogs_host(), ARG(0), ARG(1)); break;
    case 6:
        f = b1_frame_enter();
        opeu_arrival_location(ds, link_mappos, f, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        b1_frame_leave();
        break;
    case 7: opeu_music_stop(ds, link_sound); break;
    case 8: opeu_turn_pre_context(ds, link_context); break;
    case 9: opeu_screen_reset(ds, link_dialogs_host()); break;
    case 10: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 11: opeu_disk_reset(ds, link_resopen); break;
    case 12: r = opeu_title_box(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 13: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 14: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 15: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 16: r = opeu_list_dialog(ds, link_listdlg, ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2)); break;
    case 17: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 18: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 19: opeu_army_title(ds, link_dialogs_host(), ARG(0)); break;
    case 20:
        f = b1_stack_alloc(KEY_SCRATCH);
        r = (uint16_t)opeu_prompt_yes_no(ds, link_keyboard, ARG(0), f);
        b1_stack_free(KEY_SCRATCH);
        break;
    case 21: opeu_turn_active_begin(ds, link_dialogs_host()); break;
    case 22: opeu_palette_title_screen(ds, link_uiprim_host()); break;
    case 23: opeu_palette_daylight(ds, link_uiprim_host()); break;
    case 24: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    case 25: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    case 26: opeu_free_segments(ds, link_scenend); break;
    case 27: r = (uint16_t)opeu_memory_release(ds, link_emsmap); break;
    case 28: opeu_close_data_files(ds, link_scenend); break;
    case 29: opeu_application_entry(ds, link_app); break;
    case 30: r = (uint16_t)opeu_dos_create((const char *)PORT_DS(ARG(0))); break;
    case 31: r = (uint16_t)opeu_dos_exists((const char *)PORT_DS(ARG(0))); break;
    case 32: r = (uint16_t)opeu_dos_write((int16_t)ARG(0), PORT_DS(ARG(1)), ARG(2)); break;
    case 33: r = (uint16_t)opeu_dos_read((int16_t)ARG(0), PORT_DS(ARG(1)), ARG(2)); break;
    case 34: r = (uint16_t)opeu_dos_read_far((int16_t)ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 35: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 36: r = (uint16_t)opeu_dos_seek((int16_t)ARG(0), (int32_t)ARG32(1), (int16_t)ARG(3)); break;
    case 37: r = (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 38: opeu_screen_copy(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), ARG(5)); break;
    case 39: r = (uint16_t)opeu_gfx_blit(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6))); break;
    case 40: opeu_fm_wait_start(ARG(0)); break;
    case 41: r = opeu_fm_busy(); break;
    /* opeu_sound_open(0L): one null far pointer is pushed; the effects
     * pointer words the routine also reads are passed as 0. */
    case 42: r = (uint16_t)opeu_sound_open(ARG(0), ARG(1), 0, 0); break;
    case 43: opeu_fm_noop(); break;
    case 44: opeu_text_mode(); break;
    case 45: port_exit((int16_t)ARG(0)); break;
    case 46: r = (uint16_t)opeu_vga_clear(); break;
    case 47: r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), 0, 0); break;
    default: port_unhandled("settings", id, count);
    }
    return r;
}
