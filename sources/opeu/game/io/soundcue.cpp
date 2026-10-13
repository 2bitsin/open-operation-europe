// One entry point per sound-effect cue: each plays its effect.
#include "io/soundcue.hpp"

#include "common/game.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"

void opeu_sound_cue0(Game& game, HostService service) {
  opeu_effect_play(game, service, 0);
}

void opeu_sound_cue1(Game& game, HostService service) {
  opeu_effect_play(game, service, 1);
}

void opeu_sound_cue2(Game& game, HostService service) {
  opeu_effect_play(game, service, 2);
}

void opeu_sound_cue3(Game& game, HostService service) {
  opeu_effect_play(game, service, 3);
}

void opeu_sound_cue4(Game& game, HostService service) {
  opeu_effect_play(game, service, 4);
}

void opeu_sound_cue5(Game& game, HostService service) {
  opeu_effect_play(game, service, 5);
}

void opeu_sound_cue6(Game& game, HostService service) {
  opeu_effect_play(game, service, 6);
}

void opeu_sound_cue7(Game& game, HostService service) {
  opeu_effect_play(game, service, 7);
}

void opeu_control_begin(Game& game, HostService service) {
  opeu_effect_play(game, service, 8);
}

void opeu_sound_cue9(Game& game, HostService service) {
  opeu_effect_play(game, service, 9);
}

void opeu_sound_cue10(Game& game, HostService service) {
  opeu_effect_play(game, service, 10);
}

void opeu_request_wait(Game& game, HostService service) {
  opeu_effect_play(game, service, 11);
}

void opeu_sound_cue12(Game& game, HostService service) {
  opeu_effect_play(game, service, 12);
}

void opeu_pending_prepare(Game& game, HostService service) {
  opeu_effect_play(game, service, 13);
}

void opeu_sound_cue14(Game& game, HostService service) {
  opeu_effect_play(game, service, 14);
}

void opeu_sound_cue15(Game& game, HostService service) {
  opeu_effect_play(game, service, 15);
}

void opeu_sound_cue16(Game& game, HostService service) {
  opeu_effect_play(game, service, 16);
}

auto svc_soundcue(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  return svc_sound(game, call, words);
}

auto port_routine_opeu_control_begin(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_control_begin(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue0(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue0(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue1(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue1(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue2(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue2(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue3(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue3(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue4(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue4(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue5(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue5(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue6(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue6(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue7(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue7(game, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue9(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue9(game, svc_soundcue);
  return 0;
}
