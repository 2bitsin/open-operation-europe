/* Link: core/dialogs.c (1707:0002..0bd5 turn frame, context panels, speaker
 * dialogs and reports). The module takes a DialogHost structure; its events,
 * from the host branch's comment, resolved by address (symbols.json):
 *   0 bitmap_restore 0000:2674  1 bitmap_save 0000:2ed8  2 ui_box 0000:2ad2
 *   3 vga_clear 0000:2aac  4 display_prepare 06ac:007e
 *   5 picture_memory_masked 0a04:022c  6 palette_for_hour 0a04:07f4
 *   7 picture_file 0a04:01a0  8 ui_outline 0a04:065a  9 context_frame 0a04:02a0
 *  10 context_portrait 0a04:046c  11 face_draw 0a04:0602
 *  12 context_save 0480:0844  13 context_restore 0480:086a
 *  14 army_position 0480:0808  15 ui_text 0480:0484  16 army_print 0480:0bb8
 *  17 request_format 0480:0b9a  18 text_scroll_mode 0480:06f2
 *  19 text_window 0480:0704  20 text_window_reset 0480:07a4
 *  21 text_putc 0480:0944  22 choose_value 0541:0238  23 yes_no 0541:060a
 *  24 delay_units 0a8e:004c  25 music_command7 0d65:01b0  26 sound_cue16 0d92:0088
 *  27 person_lacks_side0_flag 0b73:02bc  28 person_selected_unit 145c:0ce6
 *  29 unit_selection_eligible 145c:0fc4  30 daily_context 1389:000e
 *  31 string_length 09cc:022a  32 string_append 09cc:01f6
 *  33 message_lookup 312c:000c  34 unit_side 145c:006c  35 unit_name 0b73:0188
 *  36 arrival_location 15c4:1408  37 unit_group_index 145c:0048
 *  38 find_group_record 0b73:0334  39 arrival_view 15c4:0808
 *  40 objective_side 145c:01b8  41 objective_eligible 145c:101a
 *  42 find_side_record 0b73:02ee  43 arrival_flash 15c4:0848
 *  44 opposite_side 0b73:04c8  45 unit_special 145c:04de  46 type_name 0b73:01de
 *  47 present(offset,segment): far routine read from data -> port_call. */
#include "b1_util.h"

/* mirror of src/main/core/dialogs.c:215-216 (host branch) */
typedef uint16_t (*DialogEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef struct { DialogEvent event; uint16_t load; uint16_t scratch; } DialogHost;

/* The original's near-stack buffers (at most 132 bytes, one owner active at
 * a time) live in the dialog routine's frame. The host takes one fixed DS
 * area for them: the bottom of the original 5120-byte stack (_end = ebe0h),
 * which the modeled stack (port.h, top ffe0h) never reaches in practice. */
#define DIALOGS_SCRATCH PORT_DS_END
/* Segments are the original paragraph numbers (load segment 0). */
#define DIALOGS_LOAD 0u

static uint16_t dialogs_event(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);

static const DialogHost dialogs_host = { dialogs_event, DIALOGS_LOAD, DIALOGS_SCRATCH };

void *link_dialogs_host(void)
{
    return (void *)&dialogs_host;
}

/* keyboard.c host routines take a 16-byte caller scratch area. */
#define KEY_SCRATCH 16u

static uint16_t dialogs_event(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0, f;
    switch (id) {
    case 0:
        r = (uint16_t)opeu_bitmap_restore(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4), port_far(ARG(5), ARG(6)));
        break;
    case 1:
        opeu_bitmap_save(ARG(0), ARG(1), ARG(2), ARG(3), port_far(ARG(4), ARG(5)));
        break;
    case 2: r = (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 3: r = (uint16_t)opeu_vga_clear(); break;
    case 4: opeu_display_prepare(ds, link_emsmap); break;
    case 5: opeu_picture_memory_masked(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3)); break;
    case 6: opeu_palette_for_hour(ds, link_uiprim_host()); break;
    case 7: opeu_picture_file(ds, link_uiprim_host(), ARG(0), ARG(1), (int16_t)ARG(2), ARG(3)); break;
    case 8: opeu_ui_outline(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 9: opeu_context_frame(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 10: opeu_context_portrait(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 11: opeu_face_draw(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2), (uint8_t)ARG(3)); break;
    case 12: opeu_context_save(ds, link_textout_host()); break;
    case 13: opeu_context_restore(ds, link_textout_host()); break;
    case 14: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 15: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 16: b1_cursor_print(ds, args, count); break;
    case 17: b1_request_format(ds, args, count); break;
    case 18: r = (uint16_t)opeu_text_scroll_mode(ds, link_textout_host(), (int16_t)ARG(0)); break;
    case 19:
        opeu_text_window(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1), (int16_t)ARG(2), (int16_t)ARG(3));
        break;
    case 20: opeu_text_window_reset(ds, link_textout_host()); break;
    case 21: r = opeu_text_put(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 22:
        f = b1_stack_alloc(KEY_SCRATCH);
        r = opeu_choose_value(ds, link_keyboard, ARG(0), ARG(1), (int16_t)ARG(2), ARG(3), f);
        b1_stack_free(KEY_SCRATCH);
        break;
    case 23:
        f = b1_stack_alloc(KEY_SCRATCH);
        r = (uint16_t)opeu_yes_no(ds, link_keyboard, f);
        b1_stack_free(KEY_SCRATCH);
        break;
    case 24: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 25: opeu_music_command7(ds, link_sound); break;
    case 26: opeu_sound_cue16(ds, link_soundcue); break;
    case 27: r = opeu_person_lacks_side0_flag(ds, ARG(0)); break;
    case 28: r = opeu_person_selected_unit(ds, ARG(0)); break;
    case 29: r = opeu_unit_selection_eligible(ds, ARG(0)); break;
    case 30: opeu_daily_context(ds, link_context, (uint8_t)ARG(0)); break;
    case 31: r = opeu_string_length(ds, ARG(0)); break;
    case 32: r = opeu_string_append(ds, ARG(0), ARG(1)); break;
    case 33: r = opeu_message_lookup(ds, link_msglook, (int16_t)ARG(0)); break;
    case 34: r = opeu_unit_side(ARG(0)); break;
    case 35: r = opeu_unit_name(ds, link_unitname, ARG(0), ARG(1)); break;
    case 36:
        f = b1_frame_enter();
        opeu_arrival_location(ds, link_mappos, f, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        b1_frame_leave();
        break;
    case 37: r = opeu_unit_group_index(ARG(0)); break;
    case 38: r = opeu_find_group_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 39:
        f = b1_frame_enter();
        opeu_arrival_view(ds, link_mapunit, f, ARG(0));
        b1_frame_leave();
        break;
    case 40: r = opeu_objective_side(ds, ARG(0)); break;
    case 41: r = opeu_objective_eligible(ds, ARG(0)); break;
    case 42: r = opeu_find_side_record(ds, (uint8_t)ARG(0), (uint8_t)ARG(1)); break;
    case 43:
        f = b1_frame_enter();
        opeu_side_cell_flash(ds, link_mapunit, f, (uint8_t)ARG(0), (uint8_t)ARG(1), (uint8_t)ARG(2));
        b1_frame_leave();
        break;
    case 44: r = opeu_opposite_side((uint8_t)ARG(0)); break;
    case 45: r = opeu_unit_special(ds, ARG(0)); break;
    case 46: r = opeu_type_name(ds, ARG(0), ARG(1)); break;
    case 47: r = (uint16_t)port_call(ds, (uint32_t)ARG(1) * 16u + ARG(0), 0, 0); break;
    default: port_unhandled("dialogs", id, count);
    }
    return r;
}
