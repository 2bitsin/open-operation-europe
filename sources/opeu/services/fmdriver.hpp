// The KOEI FM driver (FMDRV.COM, "OPL-II Sound Driver Version 3.30"), rewritten by hand.
//
// MAIN and OPEN call the driver through INT 66h and a far-call entry. It sequences song and sound-effect banks that
// live in the caller's memory and writes YM3812 (OPL2) registers from the PIT interrupt. This class keeps the driver's
// data model and behaviour -- the register stream it produces for a given call and tick sequence is the original's
// (probes/fmdriver-trace prints it) -- as named voice records, one function per track command, and a host interface
// instead of ports and segment registers:
//
//   WriteRegister(reg, value)  one OPL2 register write (port 388h/389h pair)
//   ReadStatus()               the OPL2 status port, only for chip detection
//   Peek(address)              one byte of the caller's memory (banks and instruments are addressed segment:offset)
//   EndOfSong(callback, queue) the client's end-of-song far callback
//
// Time: TimerTick() is one PIT interrupt (the driver programs PIT channel 0 to 1000h counts: 1193182/4096 = 291.304
// Hz). The host calls it at that rate (services/fm.cpp does so sample-accurately from the audio clock). Calls into the
// driver must not overlap a tick; the host serialises them.
//
// Widths: the arithmetic keeps the original's 8- and 16-bit wrap and its signed/unsigned multiplies and divides,
// because the register stream depends on them; the 16-bit fields are the driver's own words.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>

namespace opeu::services::detail::fmdriver {

inline constexpr auto PIT_HZ{ 1193182u };   // the PIT's input clock
inline constexpr auto PIT_DIVISOR{ 4096u }; // counts per driver tick
inline constexpr auto NO_DRIVER{ 0xFFFFu }; // the status of a call the driver does not take

// A segment:offset address in the caller's memory.
struct FarAddress {
  std::uint16_t segment{};
  std::uint16_t offset{};

  [[nodiscard]] constexpr auto IsNull() const -> bool {
    return (segment | offset) == 0;
  }
  auto operator==(FarAddress const&) const -> bool = default;
};

// One call of the driver: the function and its argument (AH, AL), a tick count (BX: a countdown to set) and an
// address (DS:DX: a song or effect bank, an instrument, a rhythm set, the end-of-song callback); the address's low
// byte also carries a function's second operand (DL: a note, a repeat mode, a volume offset, a transpose, drums).
struct Request {
  std::uint8_t  function{};
  std::uint8_t  argument{};
  std::uint16_t ticks{};
  FarAddress    address{};

  [[nodiscard]] constexpr auto Operand() const -> std::uint8_t {
    return static_cast<std::uint8_t>(address.offset);
  }
};

// What a call returns: a status (AX: the current song and repeat mode, 0, the version, NO_DRIVER), a tick count (BX:
// music or effect ticks, a countdown) and an address (DS:DX: the previous end-of-song callback). What a function does
// not return comes back as it went in.
struct Reply {
  std::uint16_t status{};
  std::uint16_t ticks{};
  FarAddress    address{};
};

// The song queue the end-of-song callback receives and returns (AL, AH, BL, BH, CH, CL): the driver then starts
// `start_song` (if nonzero) with `start_repeat` and queues `queued_song` with `queued_repeat`.
struct SongQueue {
  std::uint8_t start_song{};    // in: the queued song
  std::uint8_t current_song{};  // in only
  std::uint8_t start_repeat{};  // in: the queued repeat mode
  std::uint8_t repeat_mode{};   // in only
  std::uint8_t queued_song{};   // in: 0
  std::uint8_t queued_repeat{}; // in: the queued repeat mode
};

// The machine the driver runs on.
class FmHost {
public:
  FmHost()                                 = default;
  FmHost(FmHost const&)                    = delete;
  FmHost(FmHost&&)                         = delete;
  auto operator=(FmHost const&) -> FmHost& = delete;
  auto operator=(FmHost&&) -> FmHost&      = delete;
  virtual ~FmHost()                        = default;

  virtual auto               WriteRegister(std::uint8_t reg, std::uint8_t value) -> void  = 0;
  [[nodiscard]] virtual auto ReadStatus() -> std::uint8_t                                 = 0;
  [[nodiscard]] virtual auto Peek(FarAddress at) -> std::uint8_t                          = 0;
  [[nodiscard]] virtual auto EndOfSong(FarAddress callback, SongQueue queue) -> SongQueue = 0;
};

// Voice flags.
inline constexpr std::uint8_t VOICE_WAITING{ 0x01 };       // first event of the track not read yet
inline constexpr std::uint8_t VOICE_VIBRATO{ 0x02 };       // software vibrato (F6h/F7h) set
inline constexpr std::uint8_t VOICE_LFO_AVAILABLE{ 0x04 }; // instrument has an LFO
inline constexpr std::uint8_t VOICE_LFO_WANTED{ 0x08 };    // F8h: run the instrument LFO
inline constexpr std::uint8_t VOICE_LEGATO{ 0x20 };        // gate longer than length: tie the next note
inline constexpr std::uint8_t VOICE_SOUNDED{ 0x40 };       // keyed on since the last silence
inline constexpr std::uint8_t VOICE_KEY_ON{ 0x80 };        // key is on

// LFO flags.
inline constexpr std::uint8_t LFO_STATE{ 0x03 }; // 0 idle, 1 delay, 2 running, 3 (re)start
inline constexpr std::uint8_t LFO_DOWN{ 0x04 };  // software vibrato bends downwards
inline constexpr std::uint8_t LFO_RAMP{ 0x40 };  // software vibrato ramp in progress
inline constexpr std::uint8_t LFO_ON{ 0x80 };    // instrument LFO enabled

// The part of a voice a new track resets (the original's clear_channel).
struct VoiceTrack {
  std::uint16_t               track{}; // current track offset in the bank segment, 0 = ended
  std::uint16_t               loop{};  // loop point (track start until FEh)
  std::uint8_t                flags{ VOICE_WAITING };
  std::uint8_t                volume{};       // track volume 0-7Fh
  std::uint8_t                note{ 0xFF };   // last note (FFh none); rhythm voice: drums keyed on
  std::uint8_t                length{ 0x30 }; // default note length (ticks)
  std::uint8_t                gate{ 0x2D };   // default key-on time (ticks)
  std::uint8_t                length_count{ 1 };
  std::uint8_t                gate_count{};
  std::uint8_t                repeat_count{};
  std::uint16_t               repeat_start{};
  std::uint16_t               repeat_exit{};
  std::uint16_t               frequency{}; // unmodulated block/F-number (B0h:A0h)
  std::array<std::uint8_t, 2> level{};     // operator levels after volume
  std::array<std::uint8_t, 2> level_mod{}; // tremolo attenuation
  FarAddress                  instrument{};
  std::uint8_t                instrument_transpose{}; // signed semitones
  std::uint8_t                lfo_flags{};
  std::uint16_t               lfo_rate{};
  std::uint16_t               lfo_depth{};
  std::uint16_t               lfo_count{};
  std::uint16_t               lfo_direction{ 1 }; // +1 / -1
  std::uint16_t               lfo_phase{};
};

// One voice: its track state, then what survives a new track (the channel and the cached registers).
struct Voice : VoiceTrack {
  std::uint8_t                opl_channel{};   // 0-8
  std::uint8_t                opl_operator{};  // modulator operator offset (carrier = +3)
  std::array<std::uint8_t, 2> ksl_tl{};        // cached registers 40h/43h + operator
  std::uint8_t                fnum{};          // cached register A0h + channel
  std::uint8_t                keyon{};         // cached register B0h + channel
  std::uint8_t                volume_offset{}; // API 80h; bit 7 mutes key-on
  std::uint8_t                transpose{};     // API 81h, signed semitones

  auto ClearTrack() -> void {
    static_cast<VoiceTrack&>(*this) = VoiceTrack{};
  }
};

// The five drums of rhythm mode, indexed hi-hat, cymbal, tom, snare, bass drum: their levels from the rhythm set and
// their velocities.
struct Drums {
  std::array<std::uint8_t, 5> level{};
  std::array<std::uint8_t, 5> velocity{};
};

// What the instrument LFO does on a fixed tick.
enum class LfoAction : std::uint8_t {
  HOLD,     // nothing to modulate
  RUN,      // modulate pitch and level
  DELAYING, // count down the delay
};

class FmDriver {
public:
  // A driver not yet installed: only the countdowns and the version query work.
  explicit FmDriver(FmHost& host) : _host{ host } { }

  // Installs the driver on the chip without the chip test (an emulated chip is always there); true.
  auto Install() -> bool;
  // Installs it after the original timer-overflow chip test through ReadStatus. False when no chip answered: then only
  // the countdowns and the version query work and the other calls return NO_DRIVER.
  auto InstallDetecting() -> bool;
  // INT 66h AH=FFh / the far uninstall entry: silence the chip.
  auto Uninstall() -> void;
  // INT 66h: the full interface, FFh/FEh/F0h/F1h included.
  [[nodiscard]] auto Interrupt(Request const& request) -> Reply;
  // The far-call entry (CS:010Ah): functions 00-09h, 10-13h, 80-83h.
  [[nodiscard]] auto FarCall(Request const& request) -> Reply;
  // One PIT interrupt (291.3 Hz).
  auto TimerTick() -> void;

private:
  static constexpr auto MUSIC_VOICES{ 4uz };  // voices 0-3 play the song
  static constexpr auto EFFECT_VOICE{ 4uz };  // voices 4-5 play the sound effect
  static constexpr auto MELODY_VOICES{ 6uz }; // on OPL channels 0-5
  static constexpr auto RHYTHM_VOICE{ 6uz };  // the drums (channel 6; channels 7 and 8 are drums in rhythm mode)

  // ---- host access ----
  auto               Opl(std::uint8_t reg, std::uint8_t value) -> void;
  [[nodiscard]] auto Peek(FarAddress at) -> std::uint8_t;
  [[nodiscard]] auto PeekWord(FarAddress at) -> std::uint16_t;
  [[nodiscard]] auto EffectVoice(std::size_t n) -> Voice&;
  [[nodiscard]] auto Rhythm() -> Voice&;
  // ---- OPL registers ----
  auto VoiceKeyOn(Voice& v) -> void;
  auto VoiceKeyOff(Voice& v) -> void;
  auto VoiceSilence(Voice& v) -> void;
  auto LoadOperatorPair(FarAddress at, std::uint8_t op, std::uint8_t channel) -> void;
  auto WriteOperatorLevel(Voice& v, std::size_t which, std::uint8_t level) -> void;
  auto WriteFrequency(Voice& v, std::uint16_t frequency) -> void;
  auto SetRhythmControl(std::uint8_t keep, std::uint8_t set) -> void;
  // ---- tempo, fade, voice state ----
  auto               SetTempo(std::uint8_t bpm) -> void;
  [[nodiscard]] auto ApplyFade(std::uint8_t volume) const -> std::uint8_t;
  auto               VoiceUpdateVolume(Voice& v, std::uint8_t volume) -> void;
  auto               VoiceResetLevelModulation(Voice& v) -> void;
  auto               VoiceRestoreFrequency(Voice& v) -> void;
  auto               VoiceSetInstrument(Voice& v, FarAddress at) -> void;
  auto               VoiceStartNote(Voice& v, std::uint8_t note, std::uint8_t gate) -> void;
  auto               VoiceSound(Voice& v, std::uint8_t note) -> void;
  // ---- rhythm mode ----
  auto               WriteRhythmPairs(std::uint8_t first, std::uint8_t second, std::uint8_t value) -> void;
  auto               RhythmReset() -> void;
  [[nodiscard]] auto LoadRhythmChannel(FarAddress at, std::uint8_t n) -> FarAddress;
  auto               RhythmLoadInstruments(FarAddress at) -> void;
  [[nodiscard]] auto RhythmAttenuation(Voice const& v, std::uint8_t volume) const -> std::uint8_t;
  auto               RhythmSetVelocity(Voice const& v, std::uint8_t drums, std::uint8_t velocity) -> void;
  auto               RhythmUpdateVolume(Voice& v, std::uint8_t volume) -> void;
  auto               RhythmKeyOn(Voice& v, std::uint8_t drums) -> void;
  // ---- song and effect control ----
  auto StartTrack(Voice& v, FarAddress entry, std::size_t slot) -> void;
  auto StartSong(std::uint8_t song) -> void;
  auto KeyOffEffectVoices() -> void;
  auto StartEffect(std::uint8_t effect) -> void;
  auto StopMusic() -> void;
  auto SongFinished() -> void;
  // ---- tracks ----
  auto               StartVibrato(Voice& v, std::uint8_t depth, std::uint16_t period) -> void;
  [[nodiscard]] auto VibratoTempoPeriod(std::uint8_t rate) const -> std::uint16_t;
  auto               MelodyLfoOn(Voice& v) -> void;
  auto               MelodyLfoOff(Voice& v) -> void;
  [[nodiscard]] auto MelodyEnd(Voice& v) -> std::uint16_t;
  [[nodiscard]] auto MelodyCommand(Voice& v, FarAddress at, std::uint8_t command) -> std::uint16_t;
  [[nodiscard]] auto MelodyControl(Voice& v, FarAddress at, std::uint8_t command) -> std::uint16_t;
  [[nodiscard]] auto MelodyVoiceRead(Voice& v, FarAddress at) -> std::uint16_t;
  auto               MelodyVoiceTick(Voice& v, std::uint16_t segment) -> void;
  [[nodiscard]] auto RhythmCommand(Voice& v, FarAddress at, std::uint8_t command) -> std::uint16_t;
  [[nodiscard]] auto RhythmFlow(Voice& v, FarAddress at, std::uint8_t command) -> std::uint16_t;
  [[nodiscard]] auto RhythmRead(Voice& v, FarAddress at) -> std::uint16_t;
  auto               RhythmTick(Voice& v, std::uint16_t segment) -> void;
  // ---- LFO ----
  [[nodiscard]] auto LfoRandom(Voice const& v) -> std::uint16_t;
  [[nodiscard]] auto LfoSample(Voice& v, std::uint8_t shape) -> std::uint16_t;
  auto               ApplyPitchModulation(Voice& v, std::uint16_t value, std::int8_t depth) -> void;
  auto               ApplyLevelModulation(Voice& v, FarAddress instrument, std::uint16_t value) -> void;
  [[nodiscard]] auto StartLfo(Voice& v, FarAddress instrument, std::uint8_t shape) -> LfoAction;
  [[nodiscard]] auto AdvanceLfo(Voice& v, FarAddress instrument, std::uint8_t shape) -> LfoAction;
  auto               VoiceLfoTick(Voice& v, FarAddress instrument) -> void;
  auto               VibratoTick(Voice& v) -> void;
  auto               ModulationTick(Voice& v) -> void;
  // ---- the two tick rates ----
  auto               EffectTick() -> void;
  auto               MusicTick() -> void;
  auto               SetFadeAttenuation(std::uint8_t attenuation) -> void;
  [[nodiscard]] auto FadeTick() -> bool;
  auto               FixedTick() -> void;
  auto               UpdateSequencer() -> void;
  auto               CountDown(std::size_t n) -> void;
  // ---- the API ----
  [[nodiscard]] auto Status() const -> std::uint16_t;
  auto               ApiStopMusic(std::uint8_t speed) -> void;
  auto               ApiStopEffect() -> void;
  auto               ApiLoadMusic(FarAddress bank) -> void;
  auto               ApiLoadEffects(FarAddress bank) -> void;
  auto               ApiSetVoiceInstrument(std::uint8_t n, FarAddress instrument) -> void;
  auto               ApiPlayVoiceNote(std::uint8_t n, std::uint8_t note) -> void;
  auto               ApiSetVoiceVolume(std::uint8_t n, std::uint8_t offset) -> void;
  auto               ApiSetRhythmInstruments(FarAddress set) -> void;
  auto               ApiRhythmKey(std::uint8_t velocity, std::uint8_t drums) -> void;
  [[nodiscard]] auto MusicCall(Request const& request) -> Reply;
  [[nodiscard]] auto EffectCall(Request const& request) -> Reply;
  [[nodiscard]] auto Dispatch(Request const& request) -> Reply;
  [[nodiscard]] auto Countdown(Request const& request) -> Reply;
  auto               InitializeChip() -> void;
  [[nodiscard]] auto DetectChip() -> bool;
  auto               Reset() -> void;
  auto               Enable() -> bool;

  std::reference_wrapper<FmHost>       _host;
  std::array<Voice, MELODY_VOICES + 1> _voices{}; // [RHYTHM_VOICE] is the rhythm voice

  // timer
  std::uint16_t                _tempo_increment{}; // added to the tempo accumulator per PIT tick
  std::uint16_t                _tempo_accumulator{};
  std::uint16_t                _pending_music_ticks{}; // accumulator carries not yet sequenced
  std::uint16_t                _pending_fixed_ticks{};
  std::uint8_t                 _fixed_tick_accumulator{}; // += 36h per PIT tick
  std::uint8_t                 _bios_tick_divider{};      // every 16th PIT tick goes to the BIOS
  std::array<std::uint8_t, 2>  _countdown_accumulator{};  // += 35h per PIT tick
  std::array<std::uint16_t, 2> _countdown{};              // INT 66h F0h/F1h
  std::uint8_t                 _enabled{};                // FFh once an OPL chip was found
  std::uint8_t                 _busy{};                   // nonzero: the tick must not sequence

  // client data
  FarAddress _music{};    // song bank (API 00h)
  FarAddress _effects{};  // sound-effect bank (API 10h)
  FarAddress _callback{}; // end-of-song callback (API 09h)

  // song state
  std::uint16_t _fixed_tick_count{}; // fixed ticks since install (LFO seed)
  std::uint16_t _random_state{};     // sample-and-hold LFO generator
  std::uint16_t _music_ticks{};      // API 03h
  std::uint16_t _effect_ticks{};     // API 13h
  std::uint8_t  _tempo{};            // beats per minute (0 = stopped)
  std::uint8_t  _fade_length{};      // API 02h; 0 = no fade
  std::uint8_t  _fade_prescaler{};   // the fade advances every 4 fixed ticks
  std::uint8_t  _fade_step{};
  std::uint8_t  _fade_attenuation{}; // 0-FFh
  std::uint8_t  _repeat_mode{};      // API 04h
  std::uint8_t  _current_song{};     // 0 = none
  std::uint8_t  _queued_repeat{};    // API 08h
  std::uint8_t  _queued_song{};
  std::uint8_t  _current_effect{};

  Drums        _drums{};
  std::uint8_t _rhythm_control{}; // shadow of register BDh (AM/VIB depth)
};

}

namespace opeu::services {
using detail::fmdriver::FarAddress;
using detail::fmdriver::FmDriver;
using detail::fmdriver::FmHost;
using detail::fmdriver::Reply;
using detail::fmdriver::Request;
using detail::fmdriver::SongQueue;
}
