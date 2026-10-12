// One entry point per sound-effect cue: each plays its effect.
#include "io/soundcue.hpp"

#include "glue.hpp"
#include "io/sound.hpp"
#include "port.hpp"

void opeu_sound_cue0(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 0);
}

void opeu_sound_cue1(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 1);
}

void opeu_sound_cue2(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 2);
}

void opeu_sound_cue3(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 3);
}

void opeu_sound_cue4(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 4);
}

void opeu_sound_cue5(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 5);
}

void opeu_sound_cue6(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 6);
}

void opeu_sound_cue7(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 7);
}

void opeu_control_begin(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 8);
}

void opeu_sound_cue9(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 9);
}

void opeu_sound_cue10(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 10);
}

void opeu_request_wait(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 11);
}

void opeu_sound_cue12(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 12);
}

void opeu_pending_prepare(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 13);
}

void opeu_sound_cue14(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 14);
}

void opeu_sound_cue15(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 15);
}

void opeu_sound_cue16(std::uint8_t* ds, HostService service) {
  opeu_effect_play(ds, service, 16);
}

auto svc_soundcue(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  return svc_sound(ds, call, words);
}

auto port_routine_opeu_control_begin(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_control_begin(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue0(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue0(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue1(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue1(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue2(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue2(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue3(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue3(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue4(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue4(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue5(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue5(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue6(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue6(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue7(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue7(ds, svc_soundcue);
  return 0;
}

auto port_routine_opeu_sound_cue9(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_sound_cue9(ds, svc_soundcue);
  return 0;
}
