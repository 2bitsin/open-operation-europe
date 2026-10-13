// The music and the sound effects, through the machine's sound. sound_driver holds the driver-open result,
// sound_enabled the player's switch, music_current and effect_current the track and effect playing (0xff initially);
// an effect is played by its number + 1.
#include "io/sound.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "io/busy-wait.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

// The far data of the FM music and the effects (data/fmusic).
constexpr std::uint16_t MUSIC_SEGMENT{ 0x3d91 };
constexpr std::uint16_t MUSIC_OFFSET{ 0x0000 };
constexpr std::uint16_t EFFECTS_OFFSET{ 0x7bba };
constexpr std::uint8_t  STOP_FADE{ 20 };

} // namespace

void opeu_sound_initialize(Game& game) {
  auto& data{ Data(game) };
  data.sound_driver  = game.Machine().Sound().Open({ MUSIC_SEGMENT, MUSIC_OFFSET }, { MUSIC_SEGMENT, EFFECTS_OFFSET }) ? 1 : 0;
  data.sound_enabled = data.sound_driver != 0;
}

void opeu_sound_reset(Game& game) {
  opeu_sound_cue9(game);
  opeu_effect_finish(game);
}

void opeu_finish_side_initialization(Game& game, std::uint8_t value) {
  auto const& data{ Data(game) };
  if (data.sound_driver && data.sound_enabled) game.Machine().Sound().SetRepeatMode(value);
}

void opeu_music_stop(Game& game) {
  if (Data(game).sound_driver) game.Machine().Sound().StopMusic(STOP_FADE);
}

auto opeu_music_playing(Game& game) -> int {
  return game.Machine().Sound().MusicPlaying();
}

void opeu_music_start(Game& game, std::uint8_t track) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_music_playing(game)) {
    if (data.music_current == track) return;
    opeu_music_stop(game);
    WaitWhile([&] { return opeu_music_playing(game) != 0; });
  }
  game.Machine().Sound().PlaySong(track);
  data.music_current = track;
}

void opeu_set_turn_context(Game& game, std::uint8_t track) {
  if (Data(game).sound_enabled) {
    opeu_music_start(game, track);
    opeu_finish_side_initialization(game, 1);
  }
}

void opeu_music_command7(Game& game) {
  game.Machine().Sound().KeyOffEffectVoice(0);
}

void opeu_effect_stop(Game& game) {
  if (Data(game).sound_driver) game.Machine().Sound().StopEffect();
}

auto opeu_effect_playing(Game& game) -> int {
  if (!Data(game).sound_driver) return 0;
  return game.Machine().Sound().EffectPlaying();
}

void opeu_effect_start(Game& game, std::uint8_t effect) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_effect_playing(game)) {
    if (data.effect_current == effect) return;
    opeu_effect_stop(game);
    WaitWhile([&] { return opeu_effect_playing(game) != 0; });
  }
  game.Machine().Sound().PlayEffect(static_cast<std::uint8_t>(effect + 1));
  data.effect_current = effect;
}

void opeu_effect_play(Game& game, std::uint8_t effect) {
  if (Data(game).sound_enabled) opeu_effect_start(game, effect);
}

void opeu_effect_finish(Game& game) {
  auto const& data{ Data(game) };
  if (data.sound_driver && data.sound_enabled) {
    WaitWhile([&] { return opeu_effect_playing(game) != 0; });
    opeu_effect_stop(game);
  }
}
