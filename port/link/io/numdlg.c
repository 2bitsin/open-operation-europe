/* Link: io/numdlg.c (05a8:0574..0f59 numeric entry dialog with slider).
 * The module's service is the shared 05a8 input boundary (host-branch
 * comment, src/main/io/numdlg.c "Host model"): calls 1..8 are MENUDRAW's
 * (1 int86(0x33), 2 context save, 3 position, 4 print(format,digit),
 * 5 context restore, 7 bitmap save, 8 bitmap restore) and are forwarded to
 * link_menudraw; the dialog's own calls are
 *  10 ui_fill 0000:2fa5 (x1,y1,x2,y2,colour)
 *  11 ui_invert 0000:2b0e (x,y,w,h,mode)   [VGALIB opeu_ui_highlight]
 *  12 EMS dialog mapping 06ac:0056 opeu_ems_map_dialog
 *  13 ui_line 0000:2d16 (x1,y1,x2,y2,colour), raised by the recovered UIPRIM
 *     outline that numdlg runs with its own bridge host.
 * In the game the dialog runs from KEYBOARD (opeu_choose_value), which
 * passes its own service (link_keyboard) down; link_numdlg serves direct
 * callers. */
#include "b1_util.h"

uint16_t link_numdlg(uint8_t *ds, uint16_t call, uint16_t *words)
{
    switch (call) {
    case 1: case 2: case 3: case 4: case 5: case 7: case 8:
        return link_menudraw(ds, call, words);
    case 10: opeu_ui_fill(words[0], words[1], words[2], words[3], words[4]); break;
    case 11: opeu_ui_highlight(words[0], words[1], words[2], words[3], words[4]); break;
    case 12: opeu_ems_map_dialog(ds, link_emsmap); break;
    case 13: opeu_ui_line(words[0], words[1], words[2], words[3], words[4]); break;
    default: port_unhandled("numdlg", call, 7);
    }
    return 0;
}

/* numdlg's routines take the bare InputService above; the module builds
 * its UiHost for opeu_ui_outline internally (OutlineBridge, numdlg.c:202+),
 * so no structure host is needed. The getter returns that bridge shape
 * (mirror of src/main/io/numdlg.c:186: { InputService service; }) for any
 * caller that wants the service through hosts.h. */
typedef uint16_t (*InputService)(uint8_t *ds, uint16_t call, uint16_t *words);
typedef struct { InputService service; } OutlineBridge;
static const OutlineBridge numdlg_bridge = { link_numdlg };

void *link_numdlg_host(void)
{
    return (void *)&numdlg_bridge;
}
