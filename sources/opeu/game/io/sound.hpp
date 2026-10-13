// The music and sound-effect driver interface (INT 66h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_sound_initialize(Game& game);
void opeu_sound_reset(Game& game);
void opeu_finish_side_initialization(Game& game, std::uint8_t value);
void opeu_music_stop(Game& game);
auto opeu_music_playing(Game& game) -> int;
void opeu_music_start(Game& game, std::uint8_t track);
void opeu_set_turn_context(Game& game, std::uint8_t track);
void opeu_music_command7(Game& game);
void opeu_effect_stop(Game& game);
auto opeu_effect_playing(Game& game) -> int;
void opeu_effect_start(Game& game, std::uint8_t effect);
void opeu_effect_play(Game& game, std::uint8_t effect);
void opeu_effect_finish(Game& game);
