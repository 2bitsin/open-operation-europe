/* Link: core/scensel.c (06db:1700 opeu_new_game_setup). Events, from the
 * host branch's comment, resolved by address (symbols.json):
 *   0 army print(format[,arg]) 0480:0bb8   1 request format(dest,format,arg) 0480:0b9a
 *   2 position(x,y) 0480:0808   3 ui text(text) 0480:0484
 *   4 prompt with cancel(text,max,items) 0541:0462
 *   5 prompt without cancel(text,max,items) 0541:0470
 *   6 yes/no(text) 0541:0664 (opeu_ask_yes_no = host opeu_prompt_yes_no)
 *   7 open resource(name) 0977:000e   8 turn context(track) 0d65:00fe
 *   9 music stop 0d65:0060   10 finish side initialization(value) 0d65:0032
 *  11 VGA clear 0000:2aac   12 masked picture(x,y,handle,block) 0a04:01ce
 *  13 title palette 0a04:0884   14 string copy 09cc:01d0   15 string append 09cc:01f6
 *  16 ui box 0000:2ad2   17 wait(tenths) 0a8e:004c opeu_delay_units
 *  18 text attribute(value) 0480:0838 (host opeu_text_paper_set)
 *  19 framed text(x,y,text) 0a04:0788 (host opeu_title_box)
 *  20 DOS close 0000:0032   21 load scenario(scenario) 06db:0378
 *  22 near call to 06db:offset(offset[,arguments]):
 *     0006 setup_marker_show  0074 setup_marker_hide  0416 load_scenario_text
 *     0468 load_message_graphics  04a2 load_scenario_graphics  081c load_saved_game
 *     09c8 staff_restore_bottom  157a confirm_generals  163a scenario_frame
 *     165e scenario_message  167c scenario_clear_panel  169a scenario_clear_text
 *     16c2 scenario_description.
 *
 * Callers of opeu_new_game_setup must pass an entry SP with at least
 * 0x64c bytes reserved below it: the 1600-byte text buffer is at
 * BP-0x648 (sp-2-0x648) on the modeled DGROUP stack. */
#include "b1_util.h"

#include <stdlib.h>

#define KEY_SCRATCH 16u   /* keyboard.c: 16-byte caller scratch area */

/* 0541 prompts: the keyboard host routines take a scratch DS area. */
static uint16_t prompt_cancel(uint8_t *ds, uint16_t text, uint16_t max, uint16_t items)
{
    uint16_t s = b1_stack_alloc(KEY_SCRATCH), r;
    r = opeu_prompt_value_cancel(ds, link_keyboard, text, max, items, s);
    b1_stack_free(KEY_SCRATCH);
    return r;
}

static uint16_t prompt_required(uint8_t *ds, uint16_t text, uint16_t max, uint16_t items)
{
    uint16_t s = b1_stack_alloc(KEY_SCRATCH), r;
    r = opeu_prompt_value_required(ds, link_keyboard, text, max, items, s);
    b1_stack_free(KEY_SCRATCH);
    return r;
}

static uint16_t prompt_yes_no(uint8_t *ds, uint16_t text)
{
    uint16_t s = b1_stack_alloc(KEY_SCRATCH), r;
    r = (uint16_t)opeu_prompt_yes_no(ds, link_keyboard, text, s);
    b1_stack_free(KEY_SCRATCH);
    return r;
}

/* LOADSCEN's host entry (src/main/io/loadscen.c) takes the contents of the
 * two files the original opens by name (DS:2e8c opeu_scenario_filename,
 * DS:2ef2 opeu_map_filename). Open them as the original does (0977:000e
 * opeu_open_resource) and hand over their whole contents. */
#define SCENARIO_FILENAME 0x2e8cu
#define MAP_FILENAME      0x2ef2u

static uint8_t *read_resource(uint8_t *ds, uint16_t name, size_t *size)
{
    int16_t h = opeu_open_resource(ds, link_resopen, name);
    int32_t end = h < 0 ? 0 : opeu_dos_seek(h, 0, 2);
    uint8_t *data;
    size_t done = 0;
    if (end < 0)
        end = 0;
    data = (uint8_t *)malloc((size_t)end + 1);
    if (!data)
        port_fatal("scensel: out of memory");
    if (h >= 0) {
        opeu_dos_seek(h, 0, 0);
        while (done < (size_t)end) {
            size_t chunk = (size_t)end - done;
            uint16_t n;
            if (chunk > 0x8000u)
                chunk = 0x8000u;
            n = (uint16_t)opeu_dos_read(h, data + done, (uint16_t)chunk);   /* AX: unsigned count */
            if (n == 0 || n == 0xffffu)
                break;
            done += n;
        }
        opeu_dos_close(h);
    }
    *size = done;
    return data;
}

static void load_scenario(uint8_t *ds, uint16_t scenario)
{
    size_t scedata_size, mapinf_size;
    uint8_t *scedata = read_resource(ds, SCENARIO_FILENAME, &scedata_size);
    uint8_t *mapinf = read_resource(ds, MAP_FILENAME, &mapinf_size);
    /* The host entry returns -1 (and leaves DS untouched) when a file is
     * shorter than the records the original reads; the original has no
     * result, so nothing is reported here beyond a trace. */
    if (opeu_load_scenario(ds, (uint8_t)scenario, scedata, scedata_size, mapinf, mapinf_size) != 0)
        port_trace("link", "scensel: scenario %u: SCENARIO/MAP files shorter than the records read", scenario);
    free(scedata);
    free(mapinf);
}

/* Near calls into segment 06db, keyed by offset. */
static uint16_t near_06db(uint8_t *ds, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (ARG(0)) {
    case 0x0006: opeu_setup_marker_show(ds, link_setmark, (uint8_t)ARG(1)); break;
    case 0x0074: opeu_setup_marker_hide(ds, link_setmark, (uint8_t)ARG(1)); break;
    case 0x0416: opeu_load_scenario_text(ds, link_scenres, (uint8_t)ARG(1), ARG(2)); break;
    case 0x0468: opeu_load_message_graphics(ds, link_scenres); break;
    case 0x04a2: opeu_load_scenario_graphics(ds, link_scenres); break;
    case 0x081c: r = opeu_load_saved_game(ds, link_loadgame); break;
    case 0x09c8: opeu_staff_restore_bottom(ds, link_staffpnl); break;
    case 0x157a: opeu_confirm_generals(ds, link_genconf); break;
    case 0x163a: opeu_scenario_frame(ds, link_scenui); break;
    case 0x165e: opeu_scenario_message(ds, link_scenui, ARG(1)); break;
    case 0x167c: opeu_scenario_clear_panel(ds, link_scenui); break;
    case 0x169a: opeu_scenario_clear_text(ds, link_scenui); break;
    case 0x16c2: opeu_scenario_description(ds, link_scenui, ARG(1)); break;
    default: port_unhandled("scensel (near call 06db:offset)", ARG(0), count);
    }
    return r;
}

uint16_t link_scensel(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    switch (id) {
    case 0: b1_cursor_print(ds, args, count); break;
    case 1: b1_request_format(ds, args, count); break;
    case 2: opeu_army_position(ds, link_textout_host(), (int16_t)ARG(0), (int16_t)ARG(1)); break;
    case 3: opeu_ui_text(ds, link_textout_host(), ARG(0)); break;
    case 4: r = prompt_cancel(ds, ARG(0), ARG(1), ARG(2)); break;
    case 5: r = prompt_required(ds, ARG(0), ARG(1), ARG(2)); break;
    case 6: r = prompt_yes_no(ds, ARG(0)); break;
    case 7: r = (uint16_t)opeu_open_resource(ds, link_resopen, ARG(0)); break;
    case 8: opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0)); break;
    case 9: opeu_music_stop(ds, link_sound); break;
    case 10: opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0)); break;
    case 11: r = (uint16_t)opeu_vga_clear(); break;
    case 12: opeu_picture_file_masked(ds, link_uiprim_host(), ARG(0), ARG(1), (int16_t)ARG(2), ARG(3)); break;
    case 13: opeu_palette_title_screen(ds, link_uiprim_host()); break;
    case 14: r = opeu_string_copy(ds, ARG(0), ARG(1)); break;
    case 15: r = opeu_string_append(ds, ARG(0), ARG(1)); break;
    case 16: r = (uint16_t)opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break;
    case 17: opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0)); break;
    case 18: opeu_text_paper_set(ds, link_textout_host(), (uint8_t)ARG(0)); break;
    case 19: r = opeu_title_box(ds, link_uiprim_host(), ARG(0), ARG(1), ARG(2)); break;
    case 20: r = (uint16_t)opeu_dos_close((int16_t)ARG(0)); break;
    case 21: load_scenario(ds, ARG(0)); break;
    case 22: r = near_06db(ds, args, count); break;
    default: port_unhandled("scensel", id, count);
    }
    return r;
}
