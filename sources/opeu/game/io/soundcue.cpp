// One entry point per sound-effect cue: each plays its effect.
#include "io/soundcue.hpp"

#include "common/game.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"

void opeu_sound_cue0(Game& game) {
  opeu_effect_play(game, 0);
}

void opeu_sound_cue1(Game& game) {
  opeu_effect_play(game, 1);
}

void opeu_sound_cue2(Game& game) {
  opeu_effect_play(game, 2);
}

void opeu_sound_cue3(Game& game) {
  opeu_effect_play(game, 3);
}

void opeu_sound_cue4(Game& game) {
  opeu_effect_play(game, 4);
}

void opeu_sound_cue5(Game& game) {
  opeu_effect_play(game, 5);
}

void opeu_sound_cue6(Game& game) {
  opeu_effect_play(game, 6);
}

void opeu_sound_cue7(Game& game) {
  opeu_effect_play(game, 7);
}

void opeu_control_begin(Game& game) {
  opeu_effect_play(game, 8);
}

void opeu_sound_cue9(Game& game) {
  opeu_effect_play(game, 9);
}

void opeu_sound_cue10(Game& game) {
  opeu_effect_play(game, 10);
}

void opeu_request_wait(Game& game) {
  opeu_effect_play(game, 11);
}

void opeu_sound_cue12(Game& game) {
  opeu_effect_play(game, 12);
}

void opeu_pending_prepare(Game& game) {
  opeu_effect_play(game, 13);
}

void opeu_sound_cue14(Game& game) {
  opeu_effect_play(game, 14);
}

void opeu_sound_cue15(Game& game) {
  opeu_effect_play(game, 15);
}

void opeu_sound_cue16(Game& game) {
  opeu_effect_play(game, 16);
}

auto port_routine_opeu_control_begin(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_control_begin(game);
  return 0;
}

auto port_routine_opeu_sound_cue0(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue0(game);
  return 0;
}

auto port_routine_opeu_sound_cue1(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue1(game);
  return 0;
}

auto port_routine_opeu_sound_cue2(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue2(game);
  return 0;
}

auto port_routine_opeu_sound_cue3(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue3(game);
  return 0;
}

auto port_routine_opeu_sound_cue4(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue4(game);
  return 0;
}

auto port_routine_opeu_sound_cue5(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue5(game);
  return 0;
}

auto port_routine_opeu_sound_cue6(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue6(game);
  return 0;
}

auto port_routine_opeu_sound_cue7(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue7(game);
  return 0;
}

auto port_routine_opeu_sound_cue9(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue9(game);
  return 0;
}
