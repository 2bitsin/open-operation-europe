// One entry point per sound-effect cue 0..16; cue 8 begins a command turn, cue 11 waits on a request, cue 13
// prepares a pending report.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_cue0(Game& game);
void opeu_sound_cue1(Game& game);
void opeu_sound_cue2(Game& game);
void opeu_sound_cue3(Game& game);
void opeu_sound_cue4(Game& game);
void opeu_sound_cue5(Game& game);
void opeu_sound_cue6(Game& game);
void opeu_sound_cue7(Game& game);
void opeu_control_begin(Game& game);
void opeu_sound_cue9(Game& game);
void opeu_sound_cue10(Game& game);
void opeu_request_wait(Game& game);
void opeu_sound_cue12(Game& game);
void opeu_pending_prepare(Game& game);
void opeu_sound_cue14(Game& game);
void opeu_sound_cue15(Game& game);
void opeu_sound_cue16(Game& game);
