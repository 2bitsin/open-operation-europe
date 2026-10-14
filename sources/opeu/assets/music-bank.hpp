#pragma once
// The KOEI FM driver's banks (services/fmdriver.hpp reads them): MAIN's song bank and effect bank and the opening's
// song bank, decoded into the parts they are made of and encoded back to the same bytes (docs/asset-pack.md, "Music").
//
// A bank is a table, then its instruments (22 bytes each), its drum kits (39 bytes each) and its tracks, end to end:
//   table       a song: five words, the tracks of music voices 0-3 and of the rhythm voice; an effect: two words, the
//               tracks of effect voices 4-5; each the track's offset relative to the entry, 0 for none;
//   instrument  the operator registers 20/23h, C0h, E0/E3h, 40/43h, 60/63h, 80/83h, a transpose, and the LFO;
//   drum kit    three channels (bass drum on 6, hi-hat and snare on 7, tom and cymbal on 8): A0h, B0h, the registers;
//   track       events up to the end command FFh; command F4h selects an instrument (a rhythm track: a drum kit) by its
//               offset relative to the command.
// The events keep their bytes: the music tables (music-table.hpp) name them.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/image.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace opeu::assets::detail::music_bank {

enum class BankKind : std::uint8_t {
  SONGS,   // five tracks an entry, on music voices 0-3 and the rhythm voice
  EFFECTS, // two tracks an entry, on effect voices 4-5
};

// Which bank and how many entries its table holds (the bank does not say).
struct BankLayout {
  BankKind    kind{};
  std::size_t entries{};
};

inline constexpr auto MAIN_SONGS{
  BankLayout{ .kind = BankKind::SONGS, .entries = 19 }
};
inline constexpr auto MAIN_EFFECTS{
  BankLayout{ .kind = BankKind::EFFECTS, .entries = 16 }
};
inline constexpr auto OPENING_SONGS{
  BankLayout{ .kind = BankKind::SONGS, .entries = 3 }
};

inline constexpr auto INSTRUMENT_BYTES{ 22uz };
inline constexpr auto DRUM_KIT_BYTES{ 39uz };
inline constexpr auto SELECT_COMMAND{ std::uint8_t{ 0xF4 } }; // an instrument or a drum kit
inline constexpr auto END_COMMAND{ std::uint8_t{ 0xFF } };

[[nodiscard]] constexpr auto SlotsOf(BankKind const kind) -> std::size_t {
  return kind == BankKind::SONGS ? 5 : 2;
}

// One operator's registers: 20h (tremolo, vibrato, sustain, key scale rate, multiple), E0h (waveform), 40h (key scale
// level, level), 60h (attack, decay), 80h (sustain level, release).
struct OperatorRegisters {
  std::uint8_t character{};
  std::uint8_t waveform{};
  std::uint8_t level{};
  std::uint8_t attack_decay{};
  std::uint8_t sustain_release{};

  auto operator==(OperatorRegisters const&) const -> bool = default;
};

// A channel's two operators and its C0h (feedback, connection).
struct Voicing {
  OperatorRegisters modulator;
  OperatorRegisters carrier;
  std::uint8_t      connection{};

  auto operator==(Voicing const&) const -> bool = default;
};

struct Instrument {
  Voicing                     voicing;
  std::uint8_t                transpose{};   // semitones, two's complement
  std::uint32_t               lfo_rate{};    // a word
  std::uint8_t                lfo_shape{};   // 0 sawtooth, 1 square, 2 triangle, 3 random, 4 ramp
  std::uint8_t                pitch_depth{}; // two's complement
  std::uint8_t                depth_step{};
  std::uint8_t                level_depth{}; // two's complement
  std::array<std::uint8_t, 2> level_scale{}; // per operator
  std::uint8_t                lfo_delay{};
  std::uint8_t                lfo_enable{}; // nonzero: the instrument has an LFO

  auto operator==(Instrument const&) const -> bool = default;
};

struct DrumChannel {
  std::uint8_t frequency_low{};  // A0h
  std::uint8_t frequency_high{}; // B0h: F-number bits 8-9, block
  Voicing      voicing;

  auto operator==(DrumChannel const&) const -> bool = default;
};

// Bass drum (channel 6), hi-hat and snare (7), tom and cymbal (8).
using DrumKit = std::array<DrumChannel, 3>;

enum class TrackKind : std::uint8_t {
  MELODY,
  RHYTHM,
};

// One event: its bytes; a selection (F4h) keeps only its command and names its instrument or drum kit by index.
struct Event {
  std::vector<std::uint8_t>  bytes;
  std::optional<std::size_t> selects;

  auto operator==(Event const&) const -> bool = default;
};

struct Track {
  std::string        name; // the first entry and voice that plays it: song-01/voice-0, song-01/rhythm, effect-03/voice-4
  TrackKind          kind{};
  std::vector<Event> events;

  auto operator==(Track const&) const -> bool = default;
};

// An entry's tracks by slot (an index into the tracks), none for an empty slot.
using Entry = std::vector<std::optional<std::size_t>>;

struct MusicBank {
  BankKind                kind{};
  std::vector<Entry>      entries;
  std::vector<Instrument> instruments;
  std::vector<DrumKit>    drum_kits;
  std::vector<Track>      tracks;

  auto operator==(MusicBank const&) const -> bool = default;
};

// How many bytes the event at the front of `rest` takes (at least 1), or none when it runs past the end.
[[nodiscard]] auto EventLength(TrackKind kind, std::span<std::uint8_t const> rest) -> std::optional<std::size_t>;

// The bank's parts, or BAD_LAYOUT when they do not tile it, TRUNCATED when a track runs past its end.
[[nodiscard]] auto DecodeMusicBank(Bytes bytes, BankLayout layout) -> Decoded<MusicBank>;

// The bank's bytes: the table, instruments, drum kits and tracks laid end to end and every offset worked out.
[[nodiscard]] auto EncodeMusicBank(MusicBank const& bank) -> std::vector<std::byte>;

}

namespace opeu::assets {
using detail::music_bank::BankKind;
using detail::music_bank::BankLayout;
using detail::music_bank::DecodeMusicBank;
using detail::music_bank::DrumChannel;
using detail::music_bank::DrumKit;
using detail::music_bank::EncodeMusicBank;
using detail::music_bank::Event;
using detail::music_bank::Instrument;
using detail::music_bank::MAIN_EFFECTS;
using detail::music_bank::MAIN_SONGS;
using detail::music_bank::MusicBank;
using detail::music_bank::OPENING_SONGS;
using detail::music_bank::OperatorRegisters;
using detail::music_bank::Track;
using detail::music_bank::TrackKind;
using detail::music_bank::Voicing;
}
