/* Link: io/soundcue.c (0d92:0008..008f effect cues 0..16). Each cue calls
 * 0d65:02b0 opeu_effect_play with the service it received, so the service
 * is SOUND's INT 66h boundary (src/main/io/soundcue.c "Host model"). */
#include "link.h"

uint16_t link_soundcue(uint8_t *ds, uint16_t call, uint16_t *words)
{
    return link_sound(ds, call, words);
}
