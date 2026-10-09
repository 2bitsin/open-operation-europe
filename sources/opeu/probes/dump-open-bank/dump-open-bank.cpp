/* Writes OPEN.EXE's FM music bank (recovered/data/ofmusic.cpp of the intro:
 * tracks, voices and events in declaration order, which is the bank's layout
 * at 0710:0002) to stdout. */
#include <stdio.h>

#include "recovered.hpp"

/* main.cpp calls this. */
int opeu_dump_open_bank(void) {
  using namespace opeu::intro::detail::ofmusic;
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
