// One entry point per sound-effect cue 0..16; cue 8 begins a command turn, cue 11 waits on a request, cue 13
// prepares a pending report.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_cue0(std::uint8_t* ds, HostService service);
void opeu_sound_cue1(std::uint8_t* ds, HostService service);
void opeu_sound_cue2(std::uint8_t* ds, HostService service);
void opeu_sound_cue3(std::uint8_t* ds, HostService service);
void opeu_sound_cue4(std::uint8_t* ds, HostService service);
void opeu_sound_cue5(std::uint8_t* ds, HostService service);
void opeu_sound_cue6(std::uint8_t* ds, HostService service);
void opeu_sound_cue7(std::uint8_t* ds, HostService service);
void opeu_control_begin(std::uint8_t* ds, HostService service);
void opeu_sound_cue9(std::uint8_t* ds, HostService service);
void opeu_sound_cue10(std::uint8_t* ds, HostService service);
void opeu_request_wait(std::uint8_t* ds, HostService service);
void opeu_sound_cue12(std::uint8_t* ds, HostService service);
void opeu_pending_prepare(std::uint8_t* ds, HostService service);
void opeu_sound_cue14(std::uint8_t* ds, HostService service);
void opeu_sound_cue15(std::uint8_t* ds, HostService service);
void opeu_sound_cue16(std::uint8_t* ds, HostService service);

// The module's services: the effects go to the sound driver's service.
auto svc_soundcue(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
