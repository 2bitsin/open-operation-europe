/* Link: core/textout.c (0480:0002..0c17 key read, formatter, text console).
 * The module takes a TextHost {event, sp}. Events, from the host branch's
 * comment (src/main/core/textout.c "Host model"):
 *   0 keyboard read: INT 16h AH=00h, returns AX      port_key_read
 *   1 keyboard poll: 0000:4066 turn_input_b          opeu_turn_input_b
 *   2 0541:011e wait_confirm(value)                  opeu_wait_confirm (KEYBOARD)
 *   3 box(x,y,w,h,color): 0000:2ad2                  opeu_ui_box
 *   4 screen clear: 0000:2aac                        opeu_vga_clear
 *   5 0d65:0028 sound reset (the unused 6)           opeu_sound_reset (SOUND)
 *   6 0a8e:005c pending finish(value)                opeu_pending_finish (WAITKEY)
 *   7 glyph draw: far routine (offset, segment) read from DS:44ec/44f0,
 *     then x, y, 1, 16, attribute, bits (DS offset): 0000:28a8 opeu_draw_near
 *     (transparent) or 0000:28c5 opeu_code_0000_28c5 (opaque).
 *
 * Stack: textout keeps its automatic buffers below host->sp (ENTER/LEAVE).
 * The getter hands out the instance with sp = port_sp, the modeled SP of
 * the caller; while textout calls out through an event, port_sp is lowered
 * to textout's current sp so that nested callers (which take the host again
 * through the getter or reserve with port_stack_enter) allocate below
 * textout's live buffers. Both are restored when the event returns.
 */
#include "link.h"

/* mirror of src/main/core/textout.c:203-204 (host branch) */
typedef uint16_t (*TextEvent)(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);
typedef struct { TextEvent event; uint16_t sp; } TextHost;

static uint16_t textout_event(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count);

static TextHost textout_host = { textout_event, PORT_STACK_TOP };

void *link_textout_host(void)
{
    textout_host.sp = port_sp;
    return &textout_host;
}

static uint16_t textout_event(uint8_t *ds, uint16_t id, const uint16_t *args, uint16_t count)
{
    uint16_t r = 0;
    uint16_t saved_port_sp = port_sp, saved_host_sp = textout_host.sp;
    if (textout_host.sp < port_sp)
        port_sp = textout_host.sp;
    switch (id) {
    case 0: r = port_key_read(); break;                                 /* INT 16h AH=0 */
    case 1: r = opeu_turn_input_b(); break;                             /* 0000:4066 */
    case 2: opeu_wait_confirm(ds, link_keyboard, (int16_t)ARG(0)); break; /* 0541:011e */
    case 3: opeu_ui_box(ARG(0), ARG(1), ARG(2), ARG(3), ARG(4)); break; /* 0000:2ad2 */
    case 4: opeu_vga_clear(); break;                                    /* 0000:2aac */
    case 5: opeu_sound_reset(ds, link_sound); break;                    /* 0d65:0028 */
    case 6: opeu_pending_finish(ds, link_waitkey, (uint8_t)ARG(0)); break; /* 0a8e:005c */
    case 7: {                                                           /* (*DS:44ec[i])() */
        uint16_t off = ARG(0), seg = ARG(1);
        const uint8_t *bits = PORT_DS(ARG(7));
        if (seg == 0 && off == 0x28a8)
            opeu_draw_near(ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), bits);
        else if (seg == 0 && off == 0x28c5)
            opeu_code_0000_28c5(ARG(2), ARG(3), ARG(4), ARG(5), ARG(6), bits);
        else
            port_fatal("textout: glyph routine %04x:%04x is not a known VGALIB entry", seg, off);
        break;
    }
    default: port_unhandled("textout", id, count);
    }
    port_sp = saved_port_sp;
    textout_host.sp = saved_host_sp;
    return r;
}
