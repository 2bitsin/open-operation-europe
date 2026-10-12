// The music and sound-effect driver interface (INT 66h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_initialize(std::uint8_t* ds, HostService service);
void opeu_sound_reset(std::uint8_t* ds, HostService service);
void opeu_finish_side_initialization(std::uint8_t* ds, HostService service,
                                     std::uint8_t value);
void opeu_music_stop(std::uint8_t* ds, HostService service);
auto opeu_music_playing(std::uint8_t* ds, HostService service) -> int;
void opeu_music_start(std::uint8_t* ds, HostService service,
                      std::uint8_t track);
void opeu_set_turn_context(std::uint8_t* ds, HostService service,
                           std::uint8_t track);
void opeu_music_load(std::uint8_t* ds, HostService service, std::uint16_t bank,
                     std::uint16_t path);
void opeu_music_select(std::uint8_t* ds, HostService service,
                       std::uint8_t value);
void opeu_music_load_select(std::uint8_t* ds, HostService service,
                            std::uint16_t bank, std::uint16_t path,
                            std::uint8_t value);
void opeu_music_command7(std::uint8_t* ds, HostService service);
void opeu_effect_stop(std::uint8_t* ds, HostService service);
auto opeu_effect_playing(std::uint8_t* ds, HostService service) -> int;
void opeu_effect_load(std::uint8_t* ds, HostService service,
                      std::uint32_t data);
void opeu_effect_start(std::uint8_t* ds, HostService service,
                       std::uint8_t effect);
void opeu_effect_play(std::uint8_t* ds, HostService service,
                      std::uint8_t effect);
void opeu_effect_finish(std::uint8_t* ds, HostService service);

// The module's services: the direct calls behind its events.
auto svc_sound(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words)
    -> std::uint16_t;
