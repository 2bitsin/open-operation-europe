// The music and sound-effect driver interface (INT 66h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_initialize(Game& game, HostService service);
void opeu_sound_reset(Game& game, HostService service);
void opeu_finish_side_initialization(Game& game, HostService service, std::uint8_t value);
void opeu_music_stop(Game& game, HostService service);
auto opeu_music_playing(Game& game, HostService service) -> int;
void opeu_music_start(Game& game, HostService service, std::uint8_t track);
void opeu_set_turn_context(Game& game, HostService service, std::uint8_t track);
void opeu_music_load(Game& game, HostService service, std::uint16_t bank, std::uint16_t path);
void opeu_music_select(Game& game, HostService service, std::uint8_t value);
void opeu_music_load_select(Game& game, HostService service, std::uint16_t bank, std::uint16_t path, std::uint8_t value);
void opeu_music_command7(Game& game, HostService service);
void opeu_effect_stop(Game& game, HostService service);
auto opeu_effect_playing(Game& game, HostService service) -> int;
void opeu_effect_load(Game& game, HostService service, std::uint32_t data);
void opeu_effect_start(Game& game, HostService service, std::uint8_t effect);
void opeu_effect_play(Game& game, HostService service, std::uint8_t effect);
void opeu_effect_finish(Game& game, HostService service);

// The module's services: the direct calls behind its events.
auto svc_sound(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
