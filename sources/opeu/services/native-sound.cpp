// The native sound: each method is one FM driver request (INT 66h) with the registers the game's sound module set:
// AH the function, AL the argument, DS the data segment unless a far address is given.
#include "native-machine.hpp"

#include "port.hpp"
#include "services.hpp"

#include <utility>

namespace opeu::services::detail::native_machine {
namespace {

enum class Function : std::uint8_t {
  PLAY_SONG            = 0x01,
  STOP_MUSIC           = 0x02,
  MUSIC_STATUS         = 0x03,
  SET_REPEAT_MODE      = 0x04,
  SET_VOICE_INSTRUMENT = 0x05,
  PLAY_VOICE_NOTE      = 0x06,
  KEY_OFF_EFFECT_VOICE = 0x07,
  LOAD_EFFECTS         = 0x10,
  PLAY_EFFECT          = 0x11,
  STOP_EFFECT          = 0x12,
  EFFECT_STATUS        = 0x13,
};

// The registers of one request: AH AL, BX, DS:DX.
struct Registers {
  Function     function;
  std::uint8_t argument{};
  FarAddress   address{ .segment = PORT_DS_SEGMENT, .offset = 0 };
};

// The request's AX on return.
auto Interrupt(Registers const& registers) -> std::uint16_t {
  auto          ax{ static_cast<std::uint16_t>(std::to_underlying(registers.function) << 8 | registers.argument) };
  std::uint16_t bx{};
  auto          dx{ registers.address.offset };
  auto          ds{ registers.address.segment };
  port_fm_int66(&ax, &bx, &dx, &ds);
  return ax;
}

// A status request's answer: something is playing.
[[nodiscard]] auto Playing(Function const status) -> bool {
  return (Interrupt({ .function = status }) & 0xFF) != 0;
}

}

auto NativeSound::Open(FarAddress const music, FarAddress const effects) -> bool {
  return opeu_sound_open(music.offset, music.segment, effects.offset, effects.segment) != 0;
}

auto NativeSound::PlaySong(std::uint8_t const song) -> void {
  Interrupt({ .function = Function::PLAY_SONG, .argument = song });
}

auto NativeSound::StopMusic(std::uint8_t const fade) -> void {
  Interrupt({ .function = Function::STOP_MUSIC, .argument = fade });
}

auto NativeSound::MusicPlaying() -> bool {
  return Playing(Function::MUSIC_STATUS);
}

auto NativeSound::SetRepeatMode(std::uint8_t const mode) -> void {
  Interrupt({ .function = Function::SET_REPEAT_MODE, .argument = mode });
}

auto NativeSound::SetVoiceInstrument(std::uint8_t const voice, FarAddress const instrument) -> void {
  Interrupt({ .function = Function::SET_VOICE_INSTRUMENT, .argument = voice, .address = instrument });
}

auto NativeSound::PlayVoiceNote(std::uint8_t const voice, std::uint16_t const note) -> void {
  Interrupt({
      .function = Function::PLAY_VOICE_NOTE, .argument = voice, .address = { .segment = PORT_DS_SEGMENT, .offset = note }
  });
}

auto NativeSound::LoadEffects(FarAddress const effects) -> void {
  Interrupt({ .function = Function::LOAD_EFFECTS, .address = effects });
}

auto NativeSound::PlayEffect(std::uint8_t const effect) -> void {
  Interrupt({ .function = Function::PLAY_EFFECT, .argument = effect });
}

auto NativeSound::StopEffect() -> void {
  Interrupt({ .function = Function::STOP_EFFECT });
}

auto NativeSound::KeyOffEffectVoice(std::uint8_t const voice) -> void {
  Interrupt({ .function = Function::KEY_OFF_EFFECT_VOICE, .argument = voice });
}

auto NativeSound::EffectPlaying() -> bool {
  return Playing(Function::EFFECT_STATUS);
}

auto NativeSound::StartCountdown(std::uint16_t const ticks) -> void {
  opeu_fm_wait_start(ticks);
}

auto NativeSound::CountdownLeft() -> std::uint16_t {
  return opeu_fm_busy();
}

}
