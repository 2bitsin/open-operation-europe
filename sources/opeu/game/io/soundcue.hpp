// One entry point per sound-effect cue 0..16; cue 8 begins a command turn, cue 11 waits on a request, cue 13
// prepares a pending report.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_cue0(Game& game, HostService service);
void opeu_sound_cue1(Game& game, HostService service);
void opeu_sound_cue2(Game& game, HostService service);
void opeu_sound_cue3(Game& game, HostService service);
void opeu_sound_cue4(Game& game, HostService service);
void opeu_sound_cue5(Game& game, HostService service);
void opeu_sound_cue6(Game& game, HostService service);
void opeu_sound_cue7(Game& game, HostService service);
void opeu_control_begin(Game& game, HostService service);
void opeu_sound_cue9(Game& game, HostService service);
void opeu_sound_cue10(Game& game, HostService service);
void opeu_request_wait(Game& game, HostService service);
void opeu_sound_cue12(Game& game, HostService service);
void opeu_pending_prepare(Game& game, HostService service);
void opeu_sound_cue14(Game& game, HostService service);
void opeu_sound_cue15(Game& game, HostService service);
void opeu_sound_cue16(Game& game, HostService service);

// The module's services: the effects go to the sound driver's service.
auto svc_soundcue(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
