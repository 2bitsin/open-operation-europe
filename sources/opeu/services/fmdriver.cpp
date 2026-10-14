// The KOEI FM driver (FMDRV.COM 3.30). See fmdriver.hpp.
//
// A hand port of the driver: the same voice records, sequencer, fade, queue, callback, LFO and rhythm logic, as
// methods on named fields. Arithmetic keeps the original's widths (8/16-bit wrap, unsigned vs signed multiply/divide)
// because the register stream depends on them; each function names the original routine it corresponds to (the
// driver's offset in brackets). Nothing of the original binary is used or executed.
//
// Bank formats (from the driver's reads):
//   song bank:   song n at (n-1)*10: five words, the track offsets of music voices 0-3 and of the rhythm voice relative
//                to the entry (0 = no track);
//   effect bank: effect n at (n-1)*4: two words, the tracks of effect voices 4-5 relative to the entry;
//   instrument:  22 bytes: [0..10] operator registers 20/23, C0, E0/E3, 40/43, 60/63, 80/83; [11] transpose; [12] word
//                LFO rate; [14] LFO shape (0 saw, 1 square, 2 triangle, 3 random, 4 ramp); [15] pitch depth (signed);
//                [16] depth step; [17] level depth (signed); [18]/[19] level scale per operator; [20] LFO delay; [21]
//                LFO enable;
//   rhythm set:  three groups (BD on channel 6, HH/SD on 7, TOM/CYM on 8) of A0h value, B0h value and 11 operator bytes.
#include "fmdriver.hpp"

#include <algorithm>
#include <bit>
#include <ranges>
#include <span>
#include <utility>

namespace opeu::services::detail::fmdriver {
namespace {

// Melody track commands (bytes C0h-FFh; 00-5Fh are notes with explicit length and gate, 60h-BFh notes with the
// default length and gate).
constexpr std::uint8_t MEL_AM_DEPTH{ 0xEE };
constexpr std::uint8_t MEL_VIBRATO_DEPTH{ 0xEF };
constexpr std::uint8_t MEL_VOLUME{ 0xF0 };
constexpr std::uint8_t MEL_TEMPO_PERIOD{ 0xF1 };
constexpr std::uint8_t MEL_TEMPO{ 0xF2 };
constexpr std::uint8_t MEL_SKIP1{ 0xF3 };
constexpr std::uint8_t MEL_INSTRUMENT{ 0xF4 };
constexpr std::uint8_t MEL_SKIP2{ 0xF5 };
constexpr std::uint8_t MEL_VIBRATO_TEMPO{ 0xF6 };
constexpr std::uint8_t MEL_VIBRATO_RATE{ 0xF7 };
constexpr std::uint8_t MEL_LFO_ON{ 0xF8 };
constexpr std::uint8_t MEL_LFO_OFF{ 0xF9 };
constexpr std::uint8_t MEL_REPEAT_BREAK{ 0xFD };
constexpr std::uint8_t MEL_LOOP_POINT{ 0xFE };
constexpr std::uint8_t MEL_END{ 0xFF };

// Rhythm track commands (bytes 80h-FFh; 00-7Fh are drum events).
constexpr std::uint8_t RHY_VOLUME{ 0xF0 };
constexpr std::uint8_t RHY_SKIP1_F1{ 0xF1 };
constexpr std::uint8_t RHY_TEMPO{ 0xF2 };
constexpr std::uint8_t RHY_SKIP1_F3{ 0xF3 };
constexpr std::uint8_t RHY_INSTRUMENTS{ 0xF4 };
constexpr std::uint8_t RHY_SKIP2_F5{ 0xF5 };
constexpr std::uint8_t RHY_SKIP2_F6{ 0xF6 };
constexpr std::uint8_t RHY_SKIP2_F7{ 0xF7 };
constexpr std::uint8_t RHY_REPEAT_BREAK{ 0xFD };
constexpr std::uint8_t RHY_LOOP_POINT{ 0xFE };
constexpr std::uint8_t RHY_END{ 0xFF };

// A drum: its bit in register BDh and in the rhythm events, and its level register. In Drums order (hi-hat, cymbal,
// tom, snare, bass drum): a velocity writes the levels in this order, a volume change in the reverse order.
struct Drum {
  std::uint8_t bit;
  std::uint8_t level_register;
};
constexpr std::array<Drum, 5> DRUMS{
  {
   { 0x01, 0x51 }, // hi-hat
      { 0x02, 0x55 }, // cymbal
      { 0x04, 0x52 }, // tom
      { 0x08, 0x54 }, // snare
      { 0x10, 0x53 }, // bass drum
  }
};

// [0710] vibrato depths (index 0-12) and [072a] LFO depth steps.
constexpr std::array<std::uint16_t, 13> MODULATION_DEPTH_TABLE{ 0x0000, 0x079C, 0x0FAD, 0x1838, 0x2145, 0x2ADC, 0x3505, 0x3FC8, 0x4B2F, 0x5744, 0x6411, 0x71A1, 0x7FFF };
constexpr std::array<std::uint16_t, 16> MODULATION_STEP_TABLE{ 0x004C, 0x0072, 0x00BE, 0x010A, 0x017D, 0x02FE, 0x079C, 0x0FAD, 0x2145, 0x2ADC, 0x3FC8, 0x4B2F, 0x5744, 0x7FFF, 0x9F58, 0xEA08 };
// OPL F-numbers of C, C#, ... B.
constexpr std::array<std::uint16_t, 12> NOTE_FREQUENCY_TABLE{ 0x0157, 0x016B, 0x0181, 0x0198, 0x01B0, 0x01CA, 0x01E5, 0x0202, 0x0220, 0x0241, 0x0263, 0x0287 };

// OPL channel and modulator operator offset of each voice record.
constexpr std::array<std::uint8_t, 7> VOICE_CHANNEL{ 0, 1, 2, 3, 4, 5, 6 };
constexpr std::array<std::uint8_t, 7> VOICE_OPERATOR{ 0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x00 };

// The registers of an instrument's 11 operator bytes; the third (C0h) is per channel, the others per operator.
constexpr std::array<std::uint8_t, 11> OPERATOR_REGISTER{ 0x20, 0x23, 0xC0, 0xE0, 0xE3, 0x40, 0x43, 0x60, 0x63, 0x80, 0x83 };
constexpr auto                         CHANNEL_REGISTER{ 2uz };

// The low 8 or 16 bits of a value: the original's register widths.
constexpr auto Low8(auto const value) -> std::uint8_t {
  return static_cast<std::uint8_t>(value);
}
constexpr auto Low16(auto const value) -> std::uint16_t {
  return static_cast<std::uint16_t>(value);
}

// A sum clamped to a register's range.
constexpr auto Saturate(std::uint32_t const sum, std::uint8_t const limit) -> std::uint8_t {
  return sum > limit ? limit : Low8(sum);
}

// The address `delta` bytes on (the offset wraps within the segment).
constexpr auto At(FarAddress const base, std::size_t const delta) -> FarAddress {
  return { .segment = base.segment, .offset = Low16(base.offset + delta) };
}

constexpr auto Next(FarAddress const at, std::uint16_t const offset) -> FarAddress {
  return { .segment = at.segment, .offset = offset };
}

auto RestartLfo(Voice& v) -> void {
  v.lfo_count     = 0;
  v.lfo_phase     = 0;
  v.lfo_direction = 1;
}

auto SetLfoState(Voice& v, std::uint8_t const state) -> void {
  v.lfo_flags = Low8((v.lfo_flags & ~LFO_STATE) | state);
}

// The block, the key and the semitone of a transposed note.
struct Pitch {
  std::uint8_t  block;
  std::uint8_t  key;
  std::uint32_t semitone;
};

// [0d20] the note lookup: within C0..B7 the octave is the block; above the top octave block 7; below C0 (or at 256
// and above) block 0.
auto Transposed(std::uint16_t const total) -> Pitch {
  auto const octave{ static_cast<std::int16_t>(total) / 12 }; // idiv: truncating
  auto       semitone{ static_cast<std::int16_t>(total) % 12 };
  if ((total & 0xFF00) == 0) {
    if (octave > 7) return { .block = 7, .key = Low8(semitone + 0x54), .semitone = static_cast<std::uint32_t>(semitone) };
    return { .block = Low8(octave), .key = Low8(total), .semitone = static_cast<std::uint32_t>(semitone) };
  }
  if (semitone != 0) semitone += 12;
  return { .block = 0, .key = Low8(semitone), .semitone = static_cast<std::uint32_t>(semitone) };
}

// [0a2d] D0h: mark the repeat start; D1h-DFh: repeat the section n more times.
auto CmdRepeat(Voice& v, std::uint16_t const at, std::uint8_t const command) -> std::uint16_t {
  auto const times{ Low8(command & 0x0F) };
  if (times == 0) {
    if (v.repeat_start == 0) v.repeat_start = at;
    return at;
  }
  if (v.repeat_start == 0) return at;
  if (v.repeat_count == 0) {
    v.repeat_count = Low8(times + 1);
    v.repeat_exit  = at;
  }
  if (--v.repeat_count != 0) return v.repeat_start;
  v.repeat_start = 0;
  return at;
}

// [0c3d] FDh: on the last pass, leave the repeat at once.
auto CmdRepeatBreak(Voice& v, std::uint16_t const at) -> std::uint16_t {
  if (v.repeat_count != 1) return at;
  v.repeat_count = 0;
  v.repeat_start = 0;
  return v.repeat_exit;
}

// Restart a track at its loop point (end of track with repeat on).
auto RestartTrack(Voice& v) -> std::uint16_t {
  v.length       = 0x30;
  v.gate         = 0x2D;
  v.length_count = 1;
  v.gate_count   = 0;
  v.repeat_count = 0;
  v.repeat_start = 0;
  return v.loop;
}

// [10fe] LfoStep: phase += rate * direction. If the phase changes sign it is left unchanged and true is returned.
auto LfoStep(Voice& v) -> bool {
  auto const delta{ Low16(static_cast<std::int16_t>(v.lfo_rate) * static_cast<std::int16_t>(v.lfo_direction)) };
  auto const old{ v.lfo_phase };
  v.lfo_phase = Low16(old + delta);
  if (((old ^ v.lfo_phase) & 0x8000) == 0) return false;
  v.lfo_phase = old;
  return true;
}

// [1117] shape 0, sawtooth: on a sign change the phase is negated (the value returned is the phase before negation).
auto LfoSawtooth(Voice& v) -> std::uint16_t {
  auto const wrapped{ LfoStep(v) };
  auto const value{ v.lfo_phase };
  if (wrapped) v.lfo_phase = Low16(-v.lfo_phase);
  return value;
}

// [1120] shape 2, triangle: twice the rate; the direction reverses at the top and bottom.
auto LfoTriangle(Voice& v) -> std::uint16_t {
  auto const delta{ Low16(static_cast<std::int16_t>(Low16(v.lfo_rate << 1)) * static_cast<std::int16_t>(v.lfo_direction)) };
  auto const old{ v.lfo_phase };
  v.lfo_phase = Low16(old + delta);
  if (((old ^ v.lfo_phase) & 0x8000) != 0 && ((v.lfo_direction ^ v.lfo_phase) & 0x8000) != 0) {
    v.lfo_phase     = old;
    v.lfo_direction = Low16(-v.lfo_direction);
  }
  return v.lfo_phase;
}

// [1144] shape 4 (and the software vibrato): ramp up to 7FFFh and stay.
auto LfoRamp(Voice& v) -> std::uint16_t {
  if (v.lfo_phase == 0x7FFF) return 0x7FFF;
  if (LfoStep(v)) v.lfo_phase = 0x7FFF;
  return v.lfo_phase;
}

// Fixed ticks per half period of the square / hold time of the random shape. The original divides 7FFFh by the rate;
// a rate above 7FFFh would make the square shape divide by zero there (a fault); it counts as 1 here.
auto LfoPeriod(Voice const& v) -> std::uint16_t {
  return Low16(0x7FFFu / v.lfo_rate);
}

// [1159] shape 1, square.
auto LfoSquare(Voice& v) -> std::uint16_t {
  auto const period{ std::max(LfoPeriod(v), std::uint16_t{ 1 }) };
  v.lfo_phase = ((v.lfo_count / period) & 1) != 0 ? 0x8000 : 0x7FFF;
  return v.lfo_phase;
}

// [11d5] VoiceBendFrequency, upwards: f + f * bend/7FFFh * depth/7FFFh, renormalised into higher blocks while it
// exceeds 10 bits (at block 7 it clamps to the top F-number).
auto BendUp(std::uint16_t const frequency, std::uint16_t const bend, std::uint16_t const depth) -> std::uint16_t {
  auto const fnum{ Low16(frequency & 0x3FF) };
  auto const block{ Low16(frequency & 0x1C00) };
  auto const up{ std::uint32_t{ fnum } * bend / 0x7FFF * depth / 0x7FFF };
  auto const value{ Low16(up + fnum) };
  auto const shifts{ std::max(0, std::bit_width(value) - 10) };
  auto const room{ (0x1C00 - block) / 0x400 };
  if (shifts > room) return 0x1C00 | 0x3FF;
  return Low16((value >> shifts) | (block + (shifts * 0x400)));
}

// [11d5] downwards: f * 7FFFh / (7FFFh + |bend| * depth / 7FFFh); numerator and divisor halved when the divisor
// exceeds 16 bits.
auto BendDown(std::uint16_t const frequency, std::uint16_t const bend, std::uint16_t const depth) -> std::uint16_t {
  auto const fnum{ Low16(frequency & 0x3FF) };
  auto const down{ std::uint32_t{ Low16(-bend) } * depth / 0x7FFF };
  auto       numerator{ std::uint32_t{ fnum } * 0x7FFF };
  auto       divisor{ std::uint32_t{ Low16(down) } + 0x7FFFu };
  if (divisor > 0xFFFF) {
    divisor   >>= 1;
    numerator >>= 1;
  }
  return Low16(Low16(numerator / divisor) | (frequency & 0x1C00));
}

// The voice's frequency bent by `bend` (signed, full scale 7FFFh) times its LFO depth.
auto BentFrequency(Voice const& v, std::uint16_t const bend) -> std::uint16_t {
  return (bend & 0x8000) != 0 ? BendDown(v.frequency, bend, v.lfo_depth) : BendUp(v.frequency, bend, v.lfo_depth);
}

// [123d] the LFO value as the level modulation's shape reads it.
auto LevelShape(std::uint16_t const value, std::uint8_t const shape) -> std::uint16_t {
  switch (shape) {
  case 4:  return Low16(value << 1);
  case 1:  return (value & 0x8000) != 0 ? 0xFFFF : 0;
  case 2:  return Low16(((value & 0x8000) != 0 ? Low16(-value) : value) << 1);
  default: return value;
  }
}

// A request's registers coming back unchanged.
auto Echo(Request const& request) -> Reply {
  return { .status = Low16(request.function << 8 | request.argument), .ticks = request.ticks, .address = request.address };
}

auto IsApiFunction(std::uint8_t const function) -> bool {
  return function <= 0x09 || (function >= 0x10 && function <= 0x13) || (function >= 0x80 && function <= 0x83);
}

}

// ---- host access ----

auto FmDriver::Opl(std::uint8_t const reg, std::uint8_t const value) -> void {
  _host.get().WriteRegister(reg, value);
}

auto FmDriver::Peek(FarAddress const at) -> std::uint8_t {
  return _host.get().Peek(at);
}

auto FmDriver::PeekWord(FarAddress const at) -> std::uint16_t {
  return Low16(Peek(at) | Peek(At(at, 1)) << 8);
}

auto FmDriver::EffectVoice(std::size_t const n) -> Voice& {
  return _voices.at(EFFECT_VOICE + n);
}

auto FmDriver::Rhythm() -> Voice& {
  return _voices[RHYTHM_VOICE];
}

// ---- OPL registers ----

// [0768] VoiceKeyOn: set KEY-ON unless the voice is muted (offset bit 7).
auto FmDriver::VoiceKeyOn(Voice& v) -> void {
  if ((v.volume_offset & 0x80) != 0) return;
  v.keyon = Low8(v.keyon | 0x20);
  Opl(Low8(0xB0 + v.opl_channel), v.keyon);
  v.flags     = Low8(v.flags | VOICE_KEY_ON | VOICE_SOUNDED);
  v.lfo_flags = Low8(v.lfo_flags | LFO_STATE); // restart the LFO
}

// [078a] VoiceKeyOff.
auto FmDriver::VoiceKeyOff(Voice& v) -> void {
  v.keyon = Low8(v.keyon & 0xDF);
  Opl(Low8(0xB0 + v.opl_channel), v.keyon);
  v.flags     = Low8(v.flags & ~(VOICE_KEY_ON | VOICE_LEGATO));
  v.lfo_flags = Low8(v.lfo_flags & ~LFO_RAMP);
}

// [07a6] VoiceSilence: frequency 0, fastest release, full attenuation.
auto FmDriver::VoiceSilence(Voice& v) -> void {
  auto const op{ v.opl_operator };
  v.fnum  = 0;
  v.keyon = 0;
  Opl(Low8(0xA0 + v.opl_channel), 0);
  Opl(Low8(0xB0 + v.opl_channel), 0);
  Opl(Low8(0x80 + op), 0x0F);
  Opl(Low8(0x83 + op), 0x0F);
  Opl(Low8(0x40 + op), 0x3F);
  v.ksl_tl[0] = 0x3F;
  Opl(Low8(0x43 + op), 0x3F);
  v.ksl_tl[1] = 0x3F;
  v.flags     = Low8(v.flags & 0x1F); // key, sounded, (40h) cleared
  v.lfo_flags = Low8(v.lfo_flags & ~LFO_RAMP);
}

// [0836] LoadOperatorPair: 11 instrument bytes to the registers of one channel (20/23, C0, E0/E3, 40/43, 60/63, 80/83).
auto FmDriver::LoadOperatorPair(FarAddress const at, std::uint8_t const op, std::uint8_t const channel) -> void {
  std::ranges::for_each(std::views::iota(0uz, OPERATOR_REGISTER.size()), [&](std::size_t const i) {
    auto const target{ i == CHANNEL_REGISTER ? channel : op };
    Opl(Low8(OPERATOR_REGISTER[i] + target), Peek(At(at, i)));
  });
}

// [0892] WriteOperatorLevel: level (0 loud .. 3Fh silent) minus the tremolo attenuation, written to 40h/43h+op if it
// changed.
auto FmDriver::WriteOperatorLevel(Voice& v, std::size_t const which, std::uint8_t const level) -> void {
  auto const loudness{ Low8(~level & 0x3F) };
  auto const cut{ Low8(std::uint32_t{ loudness } * v.level_mod[which] / 0x7F) };
  auto const remaining{ loudness >= cut ? Low8(loudness - cut) : std::uint8_t{ 0 } };
  auto const old{ v.ksl_tl[which] };
  auto const value{ Low8((~remaining & 0x3F) | (old & 0xC0)) };
  if (value == old) return;
  v.ksl_tl[which] = value;
  Opl(Low8(0x40 + v.opl_operator + (3 * which)), value);
}

// [08c2] WriteFrequency: A0h/B0h+channel when the block/F-number changed; the KEY-ON bit is kept.
auto FmDriver::WriteFrequency(Voice& v, std::uint16_t const frequency) -> void {
  auto const want{ Low16(frequency & 0xDFFF) };
  auto const have{ Low16((v.fnum | v.keyon << 8) & 0xDFFF) };
  if (want == have) return;
  v.fnum = Low8(want);
  Opl(Low8(0xA0 + v.opl_channel), v.fnum);
  v.keyon = Low8((v.keyon & 0x20) | want >> 8);
  Opl(Low8(0xB0 + v.opl_channel), v.keyon);
}

// [0c0b]/[0c23] the AM and vibrato depth bits of register BDh.
auto FmDriver::SetRhythmControl(std::uint8_t const keep, std::uint8_t const set) -> void {
  _rhythm_control = Low8((_rhythm_control & keep) | set);
  Opl(0xBD, _rhythm_control);
}

// ---- tempo, fade, voice state ----

// [08f3] SetTempo: bpm (0 = keep); the timer adds bpm*464Eh/100 per PIT tick to the 16-bit tempo accumulator, one
// music tick per carry.
auto FmDriver::SetTempo(std::uint8_t const bpm) -> void {
  if (bpm == 0) return;
  _tempo             = bpm;
  _tempo_increment   = Low16(std::uint32_t{ bpm } * 0x464E / 100);
  _tempo_accumulator = 0;
}

// [0913] ApplyFade: volume plus the fade attenuation, clamped to 7Fh.
auto FmDriver::ApplyFade(std::uint8_t const volume) const -> std::uint8_t {
  auto const attenuation{ _fade_attenuation >= 0x80 ? Low8(_fade_attenuation - 0x40) : Low8(_fade_attenuation >> 1) };
  return Saturate(std::uint32_t{ volume } + attenuation, 0x7F);
}

// [0a65] VoiceUpdateVolume: operator levels from the instrument and the volume (plus offset; music voices 0-3 also the
// fade). The carrier always follows the volume, the modulator only in additive mode (C0h bit 0).
auto FmDriver::VoiceUpdateVolume(Voice& v, std::uint8_t const volume) -> void {
  auto const offset{ Saturate(std::uint32_t{ volume } + (v.volume_offset & 0x7F), 0x7F) };
  auto const attenuation{ Low8((v.opl_channel < 4 ? ApplyFade(offset) : offset) >> 1) };
  if (v.instrument.IsNull()) return;
  auto const additive{ Low8(Peek(At(v.instrument, 2)) | 2) };
  std::ranges::for_each(std::views::iota(0uz, 2uz), [&](std::size_t const i) {
    auto const base{ Low8(Peek(At(v.instrument, 5 + i)) & 0x3F) };
    auto const level{ ((additive >> i) & 1) != 0 ? Saturate(std::uint32_t{ base } + attenuation, 0x3F) : base };
    v.level[i] = level;
    WriteOperatorLevel(v, i, level);
  });
}

// [0ac2] VoiceResetLevelModulation.
auto FmDriver::VoiceResetLevelModulation(Voice& v) -> void {
  v.level_mod = {};
  VoiceUpdateVolume(v, v.volume);
}

// [0c01] VoiceRestoreFrequency: unmodulated pitch and levels.
auto FmDriver::VoiceRestoreFrequency(Voice& v) -> void {
  WriteFrequency(v, v.frequency);
  VoiceResetLevelModulation(v);
}

// [0b07] VoiceSetInstrument: program the operators from the instrument and set up its LFO.
auto FmDriver::VoiceSetInstrument(Voice& v, FarAddress const at) -> void {
  v.instrument = at;
  VoiceSilence(v);
  LoadOperatorPair(at, v.opl_operator, v.opl_channel);
  v.ksl_tl               = { Peek(At(at, 5)), Peek(At(at, 6)) };
  v.instrument_transpose = Peek(At(at, 11));
  v.lfo_flags            = Low8(v.lfo_flags & ~LFO_ON);
  v.flags                = Low8(v.flags & ~VOICE_LFO_AVAILABLE);
  if (Peek(At(at, 21)) != 0) {
    v.flags = Low8(v.flags | VOICE_LFO_AVAILABLE);
    if ((v.flags & VOICE_LFO_WANTED) != 0) {
      v.lfo_flags = Low8(v.lfo_flags | LFO_ON);
      RestartLfo(v);
    }
  }
  VoiceResetLevelModulation(v);
}

// [0d20] VoiceStartNote: note 0-5Fh (gate 0 = rest) sounded; then the length and gate counters start.
auto FmDriver::VoiceStartNote(Voice& v, std::uint8_t const note, std::uint8_t const gate) -> void {
  if (gate != 0) {
    VoiceSound(v, note);
  } else {
    v.note = 0xFF;
    if ((v.flags & VOICE_KEY_ON) != 0) VoiceKeyOff(v);
  }
  v.length_count = v.length;
  v.gate_count   = v.gate;
  v.flags        = Low8(v.flags & ~(VOICE_LEGATO | VOICE_VIBRATO));
  if (v.gate > v.length) v.flags = Low8(v.flags | VOICE_LEGATO);
}

// [0d20] the note transposed, the F-number looked up, keyed on (a legato voice keeps the key and does not retrigger
// the same note).
auto FmDriver::VoiceSound(Voice& v, std::uint8_t const note) -> void {
  auto const pitch{ Transposed(Low16(note + static_cast<std::int8_t>(v.instrument_transpose) + static_cast<std::int8_t>(v.transpose))) };
  auto const legato{ (v.flags & VOICE_LEGATO) != 0 };
  if (!legato && (v.flags & VOICE_KEY_ON) != 0) VoiceKeyOff(v);
  if (!legato || pitch.key != v.note) {
    // The original indexes its 12-entry table with the semitone; a transposed note >= 256 would index past it (into
    // code bytes). That needs transposes summing to over 160 semitones; here the index wraps instead.
    auto const frequency{ Low16(NOTE_FREQUENCY_TABLE.at(pitch.semitone % 12) | pitch.block << 10) };
    v.note = pitch.key;
    WriteFrequency(v, frequency);
    v.frequency = frequency;
    if (!legato) VoiceKeyOn(v);
    v.lfo_flags = Low8(v.lfo_flags & ~LFO_RAMP);
  }
  if ((v.flags & VOICE_VIBRATO) != 0) v.lfo_flags = Low8(v.lfo_flags | LFO_RAMP);
}

// ---- rhythm mode ----

// The same value to registers first+0..2 and second+0..2, interleaved (channels 6-8).
auto FmDriver::WriteRhythmPairs(std::uint8_t const first, std::uint8_t const second, std::uint8_t const value) -> void {
  std::ranges::for_each(std::views::iota(0, 3), [&](int const i) {
    Opl(Low8(first + i), value);
    Opl(Low8(second + i), value);
  });
}

// [07e8] RhythmReset: BDh rhythm mode with no drum keyed; channels 6-8 silenced (frequency 0, fastest release, full
// attenuation).
auto FmDriver::RhythmReset() -> void {
  Opl(0xBD, Low8((_rhythm_control & 0xE0) | 0x20));
  Rhythm().note = 0x20;
  WriteRhythmPairs(0xA6, 0xB6, 0x00);
  WriteRhythmPairs(0x90, 0x93, 0x0F);
  WriteRhythmPairs(0x50, 0x53, 0x3F);
}

// One group of a rhythm set: channel 6+n's A0h and B0h values and its operators (10h+n/13h+n); returns the address
// of the operator bytes.
auto FmDriver::LoadRhythmChannel(FarAddress const at, std::uint8_t const n) -> FarAddress {
  Opl(Low8(0xA6 + n), Peek(at));
  Opl(Low8(0xB6 + n), Low8(Peek(At(at, 1)) & 0x1F));
  auto const operators{ At(at, 2) };
  LoadOperatorPair(operators, Low8(0x10 + n), Low8(6 + n));
  return operators;
}

// [0e4d] RhythmLoadInstruments: program channels 6-8 from a rhythm set and remember each drum's base level (bass drum
// the carrier of channel 6; hi-hat and snare the modulator and carrier of 7; tom and cymbal those of 8).
auto FmDriver::RhythmLoadInstruments(FarAddress const at) -> void {
  RhythmReset();
  auto const level{ [this](FarAddress const operators, std::size_t const n) { return Low8(Peek(At(operators, n)) & 0x3F); } };
  auto const bass_drum{ LoadRhythmChannel(at, 0) };
  _drums.level[4] = level(bass_drum, 6);
  auto const hi_hat_snare{ LoadRhythmChannel(At(bass_drum, 11), 1) };
  _drums.level[0] = level(hi_hat_snare, 5);
  _drums.level[3] = level(hi_hat_snare, 6);
  auto const tom_cymbal{ LoadRhythmChannel(At(hi_hat_snare, 11), 2) };
  _drums.level[2] = level(tom_cymbal, 5);
  _drums.level[1] = level(tom_cymbal, 6);
}

// Rhythm voice volume (plus offset and fade) as an attenuation 0-3Fh.
auto FmDriver::RhythmAttenuation(Voice const& v, std::uint8_t const volume) const -> std::uint8_t {
  return Low8(ApplyFade(Saturate(std::uint32_t{ volume } + (v.volume_offset & 0x7F), 0x7F)) >> 1);
}

// [0ec5] RhythmSetVelocity: velocity for the drums in `drums`; their levels are written at once.
auto FmDriver::RhythmSetVelocity(Voice const& v, std::uint8_t const drums, std::uint8_t const velocity) -> void {
  auto const scaled{ Low8((velocity & 0x7F) >> 1) };
  auto const attenuation{ Low8(RhythmAttenuation(v, v.volume) + scaled) };
  std::ranges::for_each(std::views::iota(0uz, DRUMS.size()), [&](std::size_t const d) {
    if ((drums & DRUMS[d].bit) == 0) return;
    _drums.velocity[d] = scaled;
    Opl(DRUMS[d].level_register, Saturate(std::uint32_t{ _drums.level[d] } + attenuation, 0x3F));
  });
}

// [0f64] RhythmUpdateVolume: new rhythm volume; all five drum levels.
auto FmDriver::RhythmUpdateVolume(Voice& v, std::uint8_t const volume) -> void {
  v.volume = Low8(volume & 0x7F);
  auto const attenuation{ RhythmAttenuation(v, v.volume) };
  std::ranges::for_each(std::views::iota(0uz, DRUMS.size()) | std::views::reverse,
                        [&](std::size_t const d) { Opl(DRUMS[d].level_register, Saturate(std::uint32_t{ Low8(_drums.level[d] + _drums.velocity[d]) } + attenuation, 0x3F)); });
}

// [1094] RhythmKeyOn: retrigger the drums in `drums` through BDh: first their key bits off, then on together with the
// drums already sounding.
auto FmDriver::RhythmKeyOn(Voice& v, std::uint8_t const drums) -> void {
  auto const keyed{ (v.volume_offset & 0x80) != 0 ? std::uint8_t{ 0 } : Low8(drums & 0x1F) }; // muted: none
  if (((v.note & 0x1F) | keyed) == 0) return;
  Opl(0xBD, Low8((~keyed & v.note) | 0x20));
  v.note = Low8(v.note | keyed);
  Opl(0xBD, Low8(v.note | _rhythm_control | 0x20));
}

// ---- song and effect control ----

// A voice cleared and set to the track in `slot` of a bank entry (relative to the entry; 0 = no track).
auto FmDriver::StartTrack(Voice& v, FarAddress const entry, std::size_t const slot) -> void {
  v.ClearTrack();
  auto const relative{ PeekWord(At(entry, slot)) };
  auto const track{ relative != 0 ? Low16(relative + entry.offset) : std::uint16_t{ 0 } };
  v.loop  = track;
  v.track = track;
}

// [02cb] start_song: song n (1-based) of the song bank on voices 0-3 and the rhythm voice, tempo 120.
auto FmDriver::StartSong(std::uint8_t const song) -> void {
  if (song == 0) return;
  _current_song     = song;
  _music_ticks      = 0;
  _fade_length      = 0;
  _fade_prescaler   = 0;
  _fade_step        = 0;
  _fade_attenuation = 0;
  auto const entry{ At(_music, 10u * Low8(song - 1)) };
  std::ranges::for_each(std::views::iota(0uz, MUSIC_VOICES), [&](std::size_t const i) { StartTrack(_voices[i], entry, 2 * i); });
  StartTrack(Rhythm(), entry, 8);
  _drums.velocity = {};
  _rhythm_control = 0;
  Opl(0xBD, 0);
  SetTempo(120);
}

// [044d] KeyOffEffectVoices.
auto FmDriver::KeyOffEffectVoices() -> void {
  VoiceKeyOff(EffectVoice(0));
  VoiceKeyOff(EffectVoice(1));
}

// [046a] StartEffect: effect n (1-based) of the effect bank on voices 4-5.
auto FmDriver::StartEffect(std::uint8_t const effect) -> void {
  if (effect == 0) return;
  _current_effect = effect;
  auto const entry{ At(_effects, 4u * Low8(effect - 1)) };
  KeyOffEffectVoices();
  std::ranges::for_each(std::views::iota(0uz, 2uz), [&](std::size_t const i) {
    StartTrack(EffectVoice(i), entry, 2 * i);
    EffectVoice(i).volume = 0x0F;
  });
}

// [092e] stop_music: silence voices 0-3 and the drums.
auto FmDriver::StopMusic() -> void {
  _current_song     = 0;
  _fade_length      = 0;
  _fade_attenuation = 0;
  std::ranges::for_each(std::span{ _voices }.first(MUSIC_VOICES), [&](Voice& v) {
    VoiceSilence(v);
    v.track = 0;
  });
  RhythmReset();
}

// [0953] SongFinished: the client callback, then the queued song.
auto FmDriver::SongFinished() -> void {
  auto queue{
    SongQueue{ .start_song = _queued_song, .current_song = _current_song, .start_repeat = _queued_repeat, .repeat_mode = _repeat_mode, .queued_song = 0, .queued_repeat = _queued_repeat }
  };
  if (!_callback.IsNull()) queue = _host.get().EndOfSong(_callback, queue);
  _current_song = queue.start_song;
  if (queue.start_song != 0) {
    _repeat_mode = queue.start_repeat;
    StartSong(queue.start_song);
  }
  _queued_song   = queue.queued_song;
  _queued_repeat = queue.queued_repeat;
}

// ---- melody tracks ----

// [0b62]/[0b6d] F7h/F6h depth, rate: software vibrato (a one-shot pitch ramp after each note). `period` is the ramp
// length in fixed ticks.
auto FmDriver::StartVibrato(Voice& v, std::uint8_t const depth, std::uint16_t const period) -> void {
  auto const down{ (depth & 0x80) != 0 };
  auto const magnitude{ down ? Low8(-depth) : depth };
  v.lfo_rate  = Low16(0x8000u / period);
  v.lfo_flags = Low8(down ? v.lfo_flags | LFO_DOWN : v.lfo_flags & ~LFO_DOWN);
  v.lfo_depth = MODULATION_DEPTH_TABLE.at(magnitude > 12 ? 0 : magnitude);
  RestartLfo(v);
  v.flags = Low8(v.flags | VOICE_VIBRATO);
  VoiceRestoreFrequency(v);
}

// [0b6d] the vibrato period of F6h: the rate scaled by the tempo.
auto FmDriver::VibratoTempoPeriod(std::uint8_t const rate) const -> std::uint16_t {
  auto const ticks{ rate == 0 ? 0x100u : std::uint32_t{ rate } };
  auto const tempo{ _tempo != 0 ? std::uint32_t{ _tempo } : 1u };
  auto const period{ Low16(ticks * 0x4E2u / (tempo << 4)) };
  return period != 0 ? period : 1;
}

// [0bd8] F8h.
auto FmDriver::MelodyLfoOn(Voice& v) -> void {
  if ((v.lfo_flags & LFO_ON) != 0) return;
  v.flags = Low8(v.flags | VOICE_LFO_WANTED);
  if ((v.flags & VOICE_LFO_AVAILABLE) != 0) v.lfo_flags = Low8(v.lfo_flags | LFO_ON | LFO_STATE);
}

// [0bed] F9h.
auto FmDriver::MelodyLfoOff(Voice& v) -> void {
  if ((v.lfo_flags & LFO_ON) == 0) return;
  v.flags     = Low8(v.flags & ~VOICE_LFO_WANTED);
  v.lfo_flags = Low8(v.lfo_flags & ~LFO_ON);
  if ((v.flags & (VOICE_KEY_ON | VOICE_SOUNDED)) != 0) VoiceRestoreFrequency(v);
}

// [0c54] FFh: music voices loop when repeating.
auto FmDriver::MelodyEnd(Voice& v) -> std::uint16_t {
  VoiceKeyOff(v);
  if (v.opl_channel >= 4 || _repeat_mode == 0 || (v.flags & VOICE_WAITING) != 0) return 0;
  _music_ticks = 0;
  return RestartTrack(v);
}

// Interpret one melody command at `at` (after the command byte); returns the next track offset (0 = track ended).
// [0c91]..[0c54]
auto FmDriver::MelodyCommand(Voice& v, FarAddress const at, std::uint8_t const command) -> std::uint16_t {
  if (command < 0xD0) return at.offset; // C0-CF: no operand, ignored
  if (command < 0xE0) return CmdRepeat(v, at.offset, command);
  if (command < MEL_AM_DEPTH) return Low16(at.offset + 1); // E0-ED: one operand, ignored
  switch (command) {
  case MEL_SKIP1:        return Low16(at.offset + 1);
  case MEL_SKIP2:        return Low16(at.offset + 2);
  case MEL_REPEAT_BREAK: return CmdRepeatBreak(v, at.offset);
  case MEL_LOOP_POINT:   v.loop = at.offset; return at.offset; // [0c50]
  case MEL_END:          return MelodyEnd(v);
  case MEL_LFO_ON:       MelodyLfoOn(v); return at.offset;
  case MEL_LFO_OFF:      MelodyLfoOff(v); return at.offset;
  default:               return MelodyControl(v, at, command); // the commands with operands; FA-FC: none
  }
}

// The melody commands with operands.
auto FmDriver::MelodyControl(Voice& v, FarAddress const at, std::uint8_t const command) -> std::uint16_t {
  auto const operand{ Peek(at) };
  auto const after_one{ Low16(at.offset + 1) };
  auto const after_two{ Low16(at.offset + 2) };
  switch (command) {
  case MEL_AM_DEPTH:      SetRhythmControl(0x7F, Low8((operand & 1) << 7)); return after_one; // [0c0b] BDh bit 7
  case MEL_VIBRATO_DEPTH: SetRhythmControl(0xBF, Low8((operand & 1) << 6)); return after_one; // [0c23] BDh bit 6
  case MEL_VOLUME:
    v.volume = Low8(operand & 0x7F);
    VoiceUpdateVolume(v, v.volume);
    return after_one;                                                                                       // [0a5e]
  case MEL_TEMPO_PERIOD: SetTempo(Low8(0x1312Du / Low16((Low8(~operand) + 1u) * 0x12u))); return after_one; // [0ae6]
  case MEL_TEMPO:                                                                                           // [0ad3] music voices
    if (v.opl_channel < 4 || v.opl_channel == 6) SetTempo(operand);
    return after_one;
  case MEL_INSTRUMENT:    VoiceSetInstrument(v, Next(at, Low16(PeekWord(at) + after_two - 3))); return after_two; // [0b00]
  case MEL_VIBRATO_TEMPO: StartVibrato(v, operand, VibratoTempoPeriod(Peek(At(at, 1)))); return after_two;        // [0b6d]
  case MEL_VIBRATO_RATE:  StartVibrato(v, operand, Low16(Peek(At(at, 1)) + 1u)); return after_two;                // [0b62]
  default:                return at.offset;
  }
}

// [0ce0] MelodyVoiceRead: commands until a note or the end of the track.
auto FmDriver::MelodyVoiceRead(Voice& v, FarAddress at) -> std::uint16_t {
  // A hand loop: a track is a stream of commands up to the next note, and how many come first is known only by
  // reading them.
  for (;;) {
    auto const byte{ Peek(at) };
    ++at.offset;
    if (byte <= 0x5F) { // note, length, gate
      v.length = Peek(at);
      v.gate   = Peek(At(at, 1));
      VoiceStartNote(v, byte, v.gate);
      return Low16(at.offset + 2);
    }
    if (byte <= 0xBF) { // note with the default length and gate
      VoiceStartNote(v, Low8(byte - 0x60), v.gate);
      return at.offset;
    }
    at.offset = MelodyCommand(v, at, byte);
    if (at.offset == 0) return 0;
  }
}

// [0cbc] MelodyVoiceTick: count down the note and its gate.
auto FmDriver::MelodyVoiceTick(Voice& v, std::uint16_t const segment) -> void {
  if (v.track == 0) return;
  if (--v.length_count == 0) {
    v.track = MelodyVoiceRead(v, { .segment = segment, .offset = v.track });
    v.flags = Low8(v.flags & ~VOICE_WAITING);
    return;
  }
  if (v.gate_count != 0 && --v.gate_count == 0) VoiceKeyOff(v);
}

// ---- rhythm track ----

// One rhythm command at `at`; returns the next offset (0 = ended). [1024] table
auto FmDriver::RhythmCommand(Voice& v, FarAddress const at, std::uint8_t const command) -> std::uint16_t {
  if (command < 0xA0) { // [0ec0] 80-9F: velocity for the drums in bits 0-4
    RhythmSetVelocity(v, command, Peek(at));
    return Low16(at.offset + 1);
  }
  if (command < 0xD0) return at.offset; // A0-CF: ignored
  if (command < 0xE0) return CmdRepeat(v, at.offset, command);
  if (command < RHY_VOLUME) return Low16(at.offset + 1); // E0-EF: one operand, ignored
  switch (command) {
  case RHY_VOLUME:      RhythmUpdateVolume(v, Peek(at)); return Low16(at.offset + 1); // [0f62]
  case RHY_TEMPO:       SetTempo(Peek(at)); return Low16(at.offset + 1);              // the rhythm voice may set the tempo
  case RHY_INSTRUMENTS: {                                                             // [0fe3] relative rhythm set
    auto const after{ Low16(at.offset + 2) };
    RhythmLoadInstruments(Next(at, Low16(PeekWord(at) + after - 3)));
    RhythmUpdateVolume(v, v.volume);
    return after;
  }
  default: return RhythmFlow(v, at, command);
  }
}

// The rhythm commands that only move along the track.
auto FmDriver::RhythmFlow(Voice& v, FarAddress const at, std::uint8_t const command) -> std::uint16_t {
  switch (command) {
  case RHY_SKIP1_F1:
  case RHY_SKIP1_F3:     return Low16(at.offset + 1);
  case RHY_SKIP2_F5:
  case RHY_SKIP2_F6:
  case RHY_SKIP2_F7:     return Low16(at.offset + 2);
  case RHY_REPEAT_BREAK: return CmdRepeatBreak(v, at.offset);
  case RHY_LOOP_POINT:   v.loop = at.offset; return at.offset;
  case RHY_END:          // [0ff8]
    if (_repeat_mode == 0 || (v.flags & VOICE_WAITING) != 0) return 0;
    return RestartTrack(v);
  default: return at.offset; // F8-FC
  }
}

// [1063] RhythmRead: commands until a drum event. A drum event byte has the drums in bits 0-4, bit 5: a length byte
// follows, bit 6: a velocity byte follows.
auto FmDriver::RhythmRead(Voice& v, FarAddress at) -> std::uint16_t {
  // A hand loop: as MelodyVoiceRead, the commands before the next event are known only by reading them.
  for (;;) {
    auto const byte{ Peek(at) };
    ++at.offset;
    if ((byte & 0x80) == 0) {
      if ((byte & 0x20) != 0) {
        v.length = Peek(at);
        ++at.offset;
      }
      if ((byte & 0x40) != 0) {
        RhythmSetVelocity(v, byte, Peek(at));
        ++at.offset;
      }
      RhythmKeyOn(v, byte);
      v.length_count = v.length;
      return at.offset;
    }
    at.offset = RhythmCommand(v, at, byte);
    if (at.offset == 0) return 0;
  }
}

// [104e] RhythmTick.
auto FmDriver::RhythmTick(Voice& v, std::uint16_t const segment) -> void {
  if (v.track == 0) return;
  if (--v.length_count != 0) return;
  v.track = RhythmRead(v, { .segment = segment, .offset = v.track });
  v.flags = Low8(v.flags & ~VOICE_WAITING);
}

// ---- LFO ----

// [117a] shape 3, sample and hold: a new value of the generator x' = x * 383h mod 7FFFh (signed) every period.
auto FmDriver::LfoRandom(Voice const& v) -> std::uint16_t {
  auto const period{ static_cast<std::int16_t>(LfoPeriod(v) | 1) };
  if (static_cast<std::int16_t>(v.lfo_count) % period == 0) {
    auto const product{ std::int32_t{ static_cast<std::int16_t>(_random_state) } * 0x383 };
    _random_state = Low16(product % 0x7FFF);
  }
  return _random_state;
}

// [11ae] LfoSample: the instrument's shape; 8000h becomes 8001h.
auto FmDriver::LfoSample(Voice& v, std::uint8_t const shape) -> std::uint16_t {
  auto const value{ [&] {
    switch (shape) {
    case 1:  return LfoSquare(v);
    case 2:  return LfoTriangle(v);
    case 3:  return LfoRandom(v);
    case 4:  return LfoRamp(v);
    default: return LfoSawtooth(v);
    }
  }() };
  return value == 0x8000 ? std::uint16_t{ 0x8001 } : value;
}

// [11c8] ApplyPitchModulation: LFO value times the instrument pitch depth (signed byte / 7Fh). The original's 16-bit
// idiv faults when the quotient overflows (depth -128 at full swing); the value wraps here.
auto FmDriver::ApplyPitchModulation(Voice& v, std::uint16_t const value, std::int8_t const depth) -> void {
  auto const scaled{ std::int32_t{ static_cast<std::int16_t>(value) } * depth / 0x7F };
  WriteFrequency(v, BentFrequency(v, Low16(scaled)));
}

// [123d] ApplyLevelModulation: tremolo from the LFO value; instrument byte 17 is the depth, bytes 18/19 scale it per
// operator (0-15).
auto FmDriver::ApplyLevelModulation(Voice& v, FarAddress const instrument, std::uint16_t const value) -> void {
  auto const shaped{ LevelShape(value, Peek(At(instrument, 14))) };
  auto const depth{ static_cast<std::int8_t>(Peek(At(instrument, 17))) };
  auto const magnitude{ Low16((depth < 0 ? -depth : depth) & 0x7F) };
  auto const amount{ Low16(std::uint32_t{ depth < 0 ? Low16(~shaped) : shaped } * magnitude / 0xFFFE) };
  std::ranges::for_each(std::views::iota(0uz, 2uz), [&](std::size_t const i) {
    auto const scale{ Low16(Peek(At(instrument, 18 + i)) & 0x0F) };
    v.level_mod[i] = Low8(std::uint32_t{ amount } * scale / 0x0F);
    WriteOperatorLevel(v, i, v.level[i]);
  });
}

// [12a8] the LFO's (re)start after a key-on: a free-running phase, an immediate start or a delay of 4 fixed ticks per
// step after the first.
auto FmDriver::StartLfo(Voice& v, FarAddress const instrument, std::uint8_t const shape) -> LfoAction {
  auto const delay{ Peek(At(instrument, 20)) };
  SetLfoState(v, 1);
  if (delay == 0 && shape != 4) {
    v.lfo_count = _fixed_tick_count; // free-running phase
  } else if (delay <= 1) {
    RestartLfo(v);
  } else {
    v.lfo_count = Low16((delay - 1u) * 4u);
    return LfoAction::HOLD;
  }
  SetLfoState(v, 2);
  return LfoAction::RUN;
}

// [12a8] the instrument LFO's state machine (restart, delay, run) for one fixed tick.
auto FmDriver::AdvanceLfo(Voice& v, FarAddress const instrument, std::uint8_t const shape) -> LfoAction {
  if ((v.lfo_flags & LFO_ON) == 0) return LfoAction::HOLD;
  auto const keyed{ (v.flags & (VOICE_KEY_ON | VOICE_SOUNDED)) != 0 };
  if (keyed) {
    v.lfo_depth = MODULATION_STEP_TABLE.at(Peek(At(instrument, 16)) & 0x0F);
    v.lfo_rate  = PeekWord(At(instrument, 12));
  }
  if (!keyed || v.lfo_rate == 0) {
    v.lfo_flags = Low8(v.lfo_flags | LFO_STATE); // wait for the next key-on
    return LfoAction::HOLD;
  }
  switch (v.lfo_flags & LFO_STATE) {
  case 0: return LfoAction::HOLD; // idle
  case 1:                         // delay: count down, then start
    if (--v.lfo_count != 0) return LfoAction::DELAYING;
    RestartLfo(v);
    SetLfoState(v, 2);
    return LfoAction::RUN;
  case 2:  return LfoAction::RUN; // running
  default: return StartLfo(v, instrument, shape);
  }
}

// [12a8] VoiceLfoTick: one fixed tick of the instrument LFO; the delay countdown does not advance the LFO's count.
auto FmDriver::VoiceLfoTick(Voice& v, FarAddress const instrument) -> void {
  auto const shape{ Peek(At(instrument, 14)) };
  auto const action{ AdvanceLfo(v, instrument, shape) };
  if (action == LfoAction::DELAYING) return;
  if (action == LfoAction::RUN) {
    auto const value{ LfoSample(v, shape > 4 ? 0 : shape) };
    ApplyPitchModulation(v, value, static_cast<std::int8_t>(Peek(At(instrument, 15))));
    ApplyLevelModulation(v, instrument, value);
  }
  ++v.lfo_count;
}

// [133e] the software vibrato: one step of the pitch ramp; at its end the bent pitch stays.
auto FmDriver::VibratoTick(Voice& v) -> void {
  auto const ramp{ LfoRamp(v) };
  auto const frequency{ BentFrequency(v, (v.lfo_flags & LFO_DOWN) != 0 ? Low16(-ramp) : ramp) };
  WriteFrequency(v, frequency);
  if (v.lfo_phase != 0x7FFF) return;
  v.frequency = frequency;
  v.lfo_flags = Low8(v.lfo_flags & ~LFO_RAMP);
}

auto FmDriver::ModulationTick(Voice& v) -> void {
  if ((v.lfo_flags & LFO_RAMP) != 0) {
    VibratoTick(v);
  } else if (!v.instrument.IsNull()) {
    VoiceLfoTick(v, v.instrument);
  }
}

// ---- the two tick rates ----

// [0e0c] EffectTick: voices 4-5; the effect ends when both tracks end.
auto FmDriver::EffectTick() -> void {
  if (_current_effect == 0) return;
  auto const effects{ std::span{ _voices }.subspan(EFFECT_VOICE, 2) };
  std::ranges::for_each(effects, [&](Voice& v) { MelodyVoiceTick(v, _effects.segment); });
  ++_effect_ticks;
  if (std::ranges::any_of(effects, [](Voice const& v) { return v.track != 0; })) return;
  _current_effect = 0;
  _effect_ticks   = 0;
}

// [0dc7] MusicTick: voices 0-3 and the rhythm voice; the song ends when every track has ended.
auto FmDriver::MusicTick() -> void {
  if (_current_song != 0) {
    auto const music{ std::span{ _voices }.first(MUSIC_VOICES) };
    std::ranges::for_each(music, [&](Voice& v) { MelodyVoiceTick(v, _music.segment); });
    RhythmTick(Rhythm(), _music.segment);
    ++_music_ticks;
    if (Rhythm().track != 0 || std::ranges::any_of(music, [](Voice const& v) { return v.track != 0; })) return;
    SongFinished();
    if (_current_song != 0) return;
  }
  _tempo_increment = 0; // no song: the music clock stops
  _tempo           = 0;
}

auto FmDriver::SetFadeAttenuation(std::uint8_t const attenuation) -> void {
  if (attenuation == _fade_attenuation) return;
  _fade_attenuation = attenuation;
  std::ranges::for_each(std::span{ _voices }.first(MUSIC_VOICES), [&](Voice& v) { VoiceUpdateVolume(v, v.volume); });
  RhythmUpdateVolume(Rhythm(), Rhythm().volume);
}

// [133e] the fade-out, every 4 fixed ticks along a quadratic curve FFh * (step / length)^2; true when it has faded out
// (the music stopped and the next song started).
auto FmDriver::FadeTick() -> bool {
  auto const prescaler{ Low8(_fade_prescaler + 1) };
  if (prescaler < 4) {
    _fade_prescaler = prescaler;
    return false;
  }
  if (_fade_step >= _fade_length) {
    StopMusic();
    SongFinished();
    return true;
  }
  ++_fade_step;
  auto const attenuation{ std::uint32_t{ Low16(_fade_step * _fade_step) } * 0xFFu / Low16(_fade_length * _fade_length) };
  SetFadeAttenuation(Saturate(attenuation, 0xFF));
  _fade_prescaler = 0;
  return false;
}

// [133e] FixedTick (about 62 Hz): fade-out, effects, vibrato and LFOs.
auto FmDriver::FixedTick() -> void {
  if (_fade_length != 0 && FadeTick()) {
    ++_fixed_tick_count;
    return;
  }
  EffectTick();
  std::ranges::for_each(std::span{ _voices }.first(MELODY_VOICES), [&](Voice& v) { ModulationTick(v); });
  ++_fixed_tick_count;
}

// [09c9] UpdateSequencer: run the ticks the timer accumulated, a music tick and a fixed tick in turn.
auto FmDriver::UpdateSequencer() -> void {
  ++_busy;
  auto const rounds{ std::max(_pending_music_ticks, _pending_fixed_ticks) };
  std::ranges::for_each(std::views::iota(0, int{ rounds }), [&](int) {
    if (_pending_music_ticks != 0) {
      MusicTick();
      --_pending_music_ticks;
    }
    if (_pending_fixed_ticks != 0) {
      FixedTick();
      --_pending_fixed_ticks;
    }
  });
  --_busy;
}

// The countdowns run at 35h/256 of the PIT rate (60.3 Hz).
auto FmDriver::CountDown(std::size_t const n) -> void {
  if (_countdown_accumulator[n] + 0x35u > 0xFF && _countdown[n] != 0) --_countdown[n];
  _countdown_accumulator[n] = Low8(_countdown_accumulator[n] + 0x35);
}

// [01de] the PIT interrupt. Music ticks come from the tempo accumulator, fixed ticks at 36h/256 of the PIT rate (62.1
// Hz). Every 16th PIT tick belongs to the BIOS clock (18.2 Hz) and does not sequence; its ticks run on the next one.
auto FmDriver::TimerTick() -> void {
  auto const sum{ std::uint32_t{ _tempo_accumulator } + _tempo_increment };
  _tempo_accumulator = Low16(sum);
  if (sum > 0xFFFF) ++_pending_music_ticks;
  if (_fixed_tick_accumulator + 0x36u > 0xFF) ++_pending_fixed_ticks;
  _fixed_tick_accumulator = Low8(_fixed_tick_accumulator + 0x36);
  CountDown(0);
  CountDown(1);
  if (--_bios_tick_divider == 0) {
    _bios_tick_divider = 16;
    return;
  }
  if (_busy == 0 && (_pending_music_ticks | _pending_fixed_ticks) != 0 && _enabled != 0) UpdateSequencer();
}

// ---- the API ----

auto FmDriver::Status() const -> std::uint16_t {
  return Low16(_current_song | _repeat_mode << 8);
}

// 02h: 0 stops at once, otherwise fades out over `speed` steps.
auto FmDriver::ApiStopMusic(std::uint8_t const speed) -> void {
  _queued_song = 0;
  if (speed == 0) {
    StopMusic();
  } else {
    _fade_length = speed;
  }
}

// 12h
auto FmDriver::ApiStopEffect() -> void {
  _current_effect = 0;
  KeyOffEffectVoices();
  VoiceSilence(EffectVoice(0));
  VoiceSilence(EffectVoice(1));
  _effect_ticks = 0;
}

// 00h: the song bank.
auto FmDriver::ApiLoadMusic(FarAddress const bank) -> void {
  ApiStopMusic(0);
  _music = bank;
  auto const reset{ [](Voice& v) {
    v.volume_offset = 0;
    v.transpose     = 0;
  } };
  std::ranges::for_each(std::span{ _voices }.first(MUSIC_VOICES), reset);
  reset(Rhythm());
}

// 10h: the effect bank.
auto FmDriver::ApiLoadEffects(FarAddress const bank) -> void {
  ApiStopEffect();
  _effects      = bank;
  _effect_ticks = 0;
  KeyOffEffectVoices();
  std::ranges::for_each(std::span{ _voices }.subspan(EFFECT_VOICE, 2), [](Voice& v) {
    v.volume_offset = 0x0F;
    v.transpose     = 0;
  });
}

// 05h: effect voice n gets the instrument (null silences it).
auto FmDriver::ApiSetVoiceInstrument(std::uint8_t const n, FarAddress const instrument) -> void {
  if (n >= 2) return;
  auto& v{ EffectVoice(n) };
  VoiceSilence(v);
  if (!instrument.IsNull()) {
    v.volume = 0;
    v.flags  = Low8(v.flags | VOICE_LFO_WANTED);
    VoiceSetInstrument(v, instrument);
    return;
  }
  v.ClearTrack();
  Opl(Low8(0x40 + v.opl_operator), 0x3F);
  v.ksl_tl[0] = 0x3F;
  Opl(Low8(0x43 + v.opl_operator), 0x3F);
  v.ksl_tl[1] = 0x3F;
}

// 06h: a note (0-5Fh) on effect voice n.
auto FmDriver::ApiPlayVoiceNote(std::uint8_t const n, std::uint8_t const note) -> void {
  if (n >= 2 || note > 0x5F) return;
  auto& v{ EffectVoice(n) };
  v.flags = Low8(v.flags & 0x9D); // not legato, no vibrato, not sounded
  VoiceStartNote(v, note, 1);
  v.flags = Low8(v.flags & ~(VOICE_LEGATO | VOICE_VIBRATO));
}

// 80h: the volume offset of voice n (0-5 melody, 6 rhythm).
auto FmDriver::ApiSetVoiceVolume(std::uint8_t const n, std::uint8_t const offset) -> void {
  if (n > RHYTHM_VOICE) return;
  auto& v{ _voices[n] };
  v.volume_offset = offset;
  if (n == RHYTHM_VOICE) {
    RhythmUpdateVolume(v, v.volume);
  } else {
    VoiceUpdateVolume(v, v.volume);
  }
}

// 82h: the rhythm set (null resets rhythm mode).
auto FmDriver::ApiSetRhythmInstruments(FarAddress const set) -> void {
  if (set.IsNull()) {
    RhythmReset();
    return;
  }
  RhythmLoadInstruments(set);
  RhythmUpdateVolume(Rhythm(), Rhythm().volume);
}

// 83h: key the drums; velocity bit 7 clear: it is their velocity.
auto FmDriver::ApiRhythmKey(std::uint8_t const velocity, std::uint8_t const drums) -> void {
  if ((velocity & 0x80) == 0) RhythmSetVelocity(Rhythm(), drums, velocity);
  RhythmKeyOn(Rhythm(), drums);
}

// [0277] functions 00-09h: the song and the effect voices' instruments and notes.
auto FmDriver::MusicCall(Request const& request) -> Reply {
  auto       reply{ Echo(request) };
  auto const argument{ request.argument };
  switch (request.function) {
  case 0x00:
    ApiLoadMusic(request.address);
    reply.status = 0;
    break;
  case 0x01: // play a song
    ApiStopMusic(0);
    if (!_music.IsNull()) StartSong(argument);
    reply.status = Status();
    break;
  case 0x02:
    ApiStopMusic(argument);
    reply.status = Status();
    break;
  case 0x03:
    reply.ticks  = _music_ticks;
    reply.status = Status();
    break;
  case 0x04:
    _repeat_mode = argument;
    reply.status = Status();
    break;
  case 0x05:
    ApiSetVoiceInstrument(argument, request.address);
    reply.status = 0;
    break;
  case 0x06:
    ApiPlayVoiceNote(argument, request.Operand());
    reply.status = 0;
    break;
  case 0x07: // key off an effect voice
    if (argument < 2) VoiceKeyOff(EffectVoice(argument));
    reply.status = 0;
    break;
  case 0x08: // queue a song with a repeat mode
    _queued_song   = argument;
    _queued_repeat = request.Operand();
    reply.status   = Status();
    break;
  default: reply.address = std::exchange(_callback, request.address); break; // 09h: swap the end-of-song callback
  }
  return reply;
}

// [0277] functions 10-13h and 80-83h: the effects, the voice volumes and transposes, the drums.
auto FmDriver::EffectCall(Request const& request) -> Reply {
  auto       reply{ Echo(request) };
  auto const argument{ request.argument };
  switch (request.function) {
  case 0x10:
    ApiLoadEffects(request.address);
    reply.status = 0;
    break;
  case 0x11:
    ApiStopEffect();
    StartEffect(argument);
    break;
  case 0x12: ApiStopEffect(); break;
  case 0x13: // the effect playing and its ticks
    reply.status = Low16((reply.status & 0xFF00) | _current_effect);
    reply.ticks  = _effect_ticks;
    break;
  case 0x80: ApiSetVoiceVolume(argument, request.Operand()); break;
  case 0x81: // transpose a melody voice
    if (argument < MELODY_VOICES) _voices[argument].transpose = request.Operand();
    break;
  case 0x82: ApiSetRhythmInstruments(request.address); break;
  default:   ApiRhythmKey(argument, request.Operand()); break; // 83h
  }
  return reply;
}

// [0277] dispatch_api: functions 00-09h, 10-13h, 80-83h; the others come back unchanged.
auto FmDriver::Dispatch(Request const& request) -> Reply {
  if (!IsApiFunction(request.function)) return Echo(request);
  ++_busy;
  auto const reply{ request.function <= 0x09 ? MusicCall(request) : EffectCall(request) };
  --_busy;
  return reply;
}

auto FmDriver::FarCall(Request const& request) -> Reply {
  if (_enabled != 0) return Dispatch(request);
  auto reply{ Echo(request) };
  reply.status = NO_DRIVER;
  return reply;
}

// F0h/F1h: the countdowns; argument 0 reads, otherwise sets.
auto FmDriver::Countdown(Request const& request) -> Reply {
  auto       reply{ Echo(request) };
  auto const n{ request.function & 1uz };
  if (request.argument == 0) {
    reply.ticks = _countdown[n];
  } else {
    _countdown[n]             = request.ticks;
    _countdown_accumulator[n] = 0;
  }
  return reply;
}

auto FmDriver::Interrupt(Request const& request) -> Reply {
  switch (request.function) {
  case 0xFF: Uninstall(); return Echo(request);
  case 0xFE: { // version 3.3; the high byte: enabled
    auto reply{ Echo(request) };
    reply.status = Low16(_enabled << 8 | 0x33);
    return reply;
  }
  case 0xF0:
  case 0xF1: return Countdown(request);
  default:   return FarCall(request);
  }
}

// [1402] initialize_opl: waveform select on, note select, no rhythm; every voice cleared and silenced.
auto FmDriver::InitializeChip() -> void {
  Opl(0x01, 0x20);
  Opl(0x08, 0x40);
  Opl(0xBD, 0x00);
  std::ranges::for_each(std::span{ _voices }.first(MELODY_VOICES), [&](Voice& v) {
    v.ClearTrack();
    VoiceSilence(v);
  });
  Rhythm().ClearTrack();
  RhythmReset();
}

// [14c6] detect_opl: the AdLib timer test (timer 1 must overflow).
auto FmDriver::DetectChip() -> bool {
  Opl(0x04, 0x60);
  Opl(0x04, 0x80);
  auto const before{ _host.get().ReadStatus() };
  Opl(0x02, 0xFF);
  Opl(0x04, 0x21);
  // the original waits here (30000 loop iterations, > 80 us)
  auto const after{ _host.get().ReadStatus() };
  Opl(0x04, 0x60);
  Opl(0x04, 0x80);
  return (before & 0xE0) == 0 && (after & 0xE0) == 0xC0;
}

// The resident program before its installer ran: every field cleared, the voices on their channels.
auto FmDriver::Reset() -> void {
  *this = FmDriver{ _host.get() };
  std::ranges::for_each(std::views::iota(0uz, _voices.size()), [&](std::size_t const i) {
    _voices[i].opl_channel  = VOICE_CHANNEL[i];
    _voices[i].opl_operator = VOICE_OPERATOR[i];
  });
  _bios_tick_divider = 1;
  _random_state      = 0x037D;
  _tempo             = 120;
}

auto FmDriver::Enable() -> bool {
  _enabled = 0xFF;
  ++_busy;
  InitializeChip();
  --_busy;
  return true;
}

auto FmDriver::Install() -> bool {
  Reset();
  return Enable();
}

auto FmDriver::InstallDetecting() -> bool {
  Reset();
  return DetectChip() && Enable(); // without a chip the driver stays resident without a sequencer
}

// [01a5] uninstall_driver: the chip is reset; the sequencer stays blocked.
auto FmDriver::Uninstall() -> void {
  if (_enabled == 0) return;
  ++_busy;
  InitializeChip();
}

}
