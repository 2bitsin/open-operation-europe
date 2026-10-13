// What the game plays: the requests it makes of the FM driver (fmdriver.hpp), one method per request, and the
// driver's countdown the game paces itself with. Banks and effect tables are far memory until B7.
#pragma once

#include "fmdriver.hpp"

#include <cstdint>

namespace opeu::services::detail::machine_sound {

class Sound {
public:
  Sound()                                = default;
  Sound(Sound const&)                    = delete;
  Sound(Sound&&)                         = delete;
  auto operator=(Sound const&) -> Sound& = delete;
  auto operator=(Sound&&) -> Sound&      = delete;
  virtual ~Sound()                       = default;

  // Loads the music bank and the effects; true when the driver took both.
  virtual auto Open(FarAddress music, FarAddress effects) -> bool = 0;
  // ---- music
  virtual auto               PlaySong(std::uint8_t song) -> void                                   = 0;
  virtual auto               StopMusic(std::uint8_t fade) -> void                                  = 0;
  [[nodiscard]] virtual auto MusicPlaying() -> bool                                                = 0;
  virtual auto               SetRepeatMode(std::uint8_t mode) -> void                              = 0;
  virtual auto               SetVoiceInstrument(std::uint8_t voice, FarAddress instrument) -> void = 0;
  virtual auto               PlayVoiceNote(std::uint8_t voice, std::uint16_t note) -> void         = 0;
  // ---- effects
  virtual auto               LoadEffects(FarAddress effects) -> void       = 0;
  virtual auto               PlayEffect(std::uint8_t effect) -> void       = 0;
  virtual auto               StopEffect() -> void                          = 0;
  virtual auto               KeyOffEffectVoice(std::uint8_t voice) -> void = 0;
  [[nodiscard]] virtual auto EffectPlaying() -> bool                       = 0;
  // ---- the countdown (60.3 Hz)
  virtual auto               StartCountdown(std::uint16_t ticks) -> void = 0;
  [[nodiscard]] virtual auto CountdownLeft() -> std::uint16_t            = 0;
};

}

namespace opeu::services {
using detail::machine_sound::Sound;
}
