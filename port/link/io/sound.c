/* Link: io/sound.c (0d65:0002..02d7 INT 66h music/effect interface).
 * Service calls, from the host branch's comment (src/main/io/sound.c):
 *   0 driver open (driver off/seg, effects off/seg): 0000:4074 opeu_sound_open
 *   1 int86(0x66): seven REGS words in/out, returns AX
 *   2 int86x(0x66): REGS then four SREGS words (es cs ss ds) in/out
 *   3 segread: four SREGS words out
 * The int86 call sites issue AH = 1 (music start), 2 (stop), 3 (status),
 * 4 (loop), 5 (load, int86x), 6 (select), 7, 10h (effect data, int86x),
 * 11h (effect start), 12h (effect stop), 13h (effect status); each goes to
 * the native driver (port_fm_function) with the registers it reads. */
#include "link.h"

#define SOUND_CODE_SEGMENT 0x0d65u

static uint16_t sound_int66(uint16_t *w, uint16_t ds_seg)
{
    uint8_t ah = (uint8_t)(w[0] >> 8), al = (uint8_t)w[0];
    uint16_t r = port_fm_function(ah, al, w[1], w[3], ds_seg);
    if (ah == 0xf0 || ah == 0xf1) {
        if (al == 0)
            w[1] = r;                    /* countdown read returns BX */
    } else {
        w[0] = r;
        if (ah == 0x03)
            w[1] = 0;                    /* BX: music counter, idle */
    }
    w[6] = 0;
    return w[0];
}

uint16_t link_sound(uint8_t *ds, uint16_t call, uint16_t *words)
{
    (void)ds;
    switch (call) {
    case 0: return (uint16_t)opeu_sound_open(words[0], words[1], words[2], words[3]);
    case 1: return sound_int66(words, PORT_DS_SEGMENT);
    case 2: return sound_int66(words, words[10]);
    case 3:
        words[0] = PORT_DS_SEGMENT;      /* ES */
        words[1] = SOUND_CODE_SEGMENT;   /* CS */
        words[2] = PORT_DS_SEGMENT;      /* SS */
        words[3] = PORT_DS_SEGMENT;      /* DS */
        return 0;
    default: port_unhandled("sound", call, 0);
    }
    return 0;
}
