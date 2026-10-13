// The music and sound-effect driver interface (INT 66h). sound_driver holds the driver-open result, sound_enabled the
// player's switch, music_current and effect_current the track and effect playing (0xff initially). Music requests
// are AH=1 start, 2 stop, 3 status, 4 control, 7; effect requests are AH=11h start (AL=effect+1), 12h stop, 13h
// status.
#include "io/sound.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "io/busy-wait.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <utility>

using namespace opeu::game;

namespace {

enum class Request : std::uint8_t {
  MUSIC_START   = 0x01,
  MUSIC_STOP    = 0x02,
  MUSIC_STATUS  = 0x03,
  MUSIC_CONTROL = 0x04,
  MUSIC_7       = 0x07,
  EFFECT_START  = 0x11,
  EFFECT_STOP   = 0x12,
  EFFECT_STATUS = 0x13,
};

// The far data of the FM music and the effects (data/fmusic).
constexpr std::uint16_t MUSIC_SEGMENT{ 0x3d91 };
constexpr std::uint16_t MUSIC_OFFSET{ 0x0000 };
constexpr std::uint16_t EFFECTS_OFFSET{ 0x7bba };
constexpr std::uint8_t  STOP_FADE{ 20 };

// INT 66h with AH = `request` and AL = `argument`, DS the data segment; answers AX. The one driver call of the music
// and the effects: the typed helpers below are its call shapes.
auto SoundInterrupt(Request request, std::uint8_t argument) -> std::uint16_t {
  std::array<std::uint16_t, 7> registers{ static_cast<std::uint16_t>(std::to_underlying(request) << 8 | argument) };
  std::uint16_t                data_segment{ PORT_DS_SEGMENT };
  return port_sound_int66(registers.data(), &data_segment);
}

auto Send(Game&, Request request, std::uint8_t argument = 0) -> void {
  SoundInterrupt(request, argument);
}

// Whether a status request answers a nonzero AL.
auto Busy(Game&, Request status) -> bool {
  return (SoundInterrupt(status, 0) & 0xff) != 0;
}
} // namespace

void opeu_sound_initialize(Game& game) {
  auto& data{ Data(game) };
  data.sound_driver  = static_cast<std::uint16_t>(opeu_sound_open(MUSIC_OFFSET, MUSIC_SEGMENT, EFFECTS_OFFSET, MUSIC_SEGMENT));
  data.sound_enabled = data.sound_driver != 0;
}

void opeu_sound_reset(Game& game) {
  opeu_sound_cue9(game);
  opeu_effect_finish(game);
}

void opeu_finish_side_initialization(Game& game, std::uint8_t value) {
  auto const& data{ Data(game) };
  if (data.sound_driver && data.sound_enabled) Send(game, Request::MUSIC_CONTROL, value);
}

void opeu_music_stop(Game& game) {
  if (Data(game).sound_driver) Send(game, Request::MUSIC_STOP, STOP_FADE);
}

auto opeu_music_playing(Game& game) -> int {
  return Busy(game, Request::MUSIC_STATUS);
}

void opeu_music_start(Game& game, std::uint8_t track) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_music_playing(game)) {
    if (data.music_current == track) return;
    opeu_music_stop(game);
    WaitWhile([&] { return opeu_music_playing(game) != 0; });
  }
  Send(game, Request::MUSIC_START, track);
  data.music_current = track;
}

void opeu_set_turn_context(Game& game, std::uint8_t track) {
  if (Data(game).sound_enabled) {
    opeu_music_start(game, track);
    opeu_finish_side_initialization(game, 1);
  }
}

void opeu_music_command7(Game& game) {
  Send(game, Request::MUSIC_7);
}

void opeu_effect_stop(Game& game) {
  if (Data(game).sound_driver) Send(game, Request::EFFECT_STOP);
}

auto opeu_effect_playing(Game& game) -> int {
  if (!Data(game).sound_driver) return 0;
  return Busy(game, Request::EFFECT_STATUS);
}

void opeu_effect_start(Game& game, std::uint8_t effect) {
  auto& data{ Data(game) };
  if (!data.sound_driver) return;
  if (opeu_effect_playing(game)) {
    if (data.effect_current == effect) return;
    opeu_effect_stop(game);
    WaitWhile([&] { return opeu_effect_playing(game) != 0; });
  }
  Send(game, Request::EFFECT_START, static_cast<std::uint8_t>(effect + 1));
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
