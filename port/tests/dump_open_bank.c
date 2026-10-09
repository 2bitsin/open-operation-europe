/* Writes OPEN.EXE's FM music bank (src/open/data/ofmusic.c, compiled
 * natively: tracks, voices and events in declaration order, which is the
 * bank's layout at 0710:0002) to stdout, for port/tests/fmdriver_diff.py. */
#include <stdio.h>

#include "types.h"

extern uint16_t opeu_music_tracks[3][5];
extern unsigned char opeu_music_voices[447];
extern unsigned char opeu_music_events[8379];

int main(void)
{
    int song, track;
    for (song = 0; song < 3; song++)
        for (track = 0; track < 5; track++) {
            putchar(opeu_music_tracks[song][track] & 0xff);
            putchar(opeu_music_tracks[song][track] >> 8);
        }
    fwrite(opeu_music_voices, 1, sizeof opeu_music_voices, stdout);
    fwrite(opeu_music_events, 1, sizeof opeu_music_events, stdout);
    return 0;
}
