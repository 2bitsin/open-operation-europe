// INT 66h: the KOEI FM driver (FMDRV) for the port.
//
// The driver's logic is the native rewrite in fmdriver.cpp; its register writes go to the OPL2 chip (opl.hpp, ymfm)
// and the chip's samples to the SDL3 audio stream. This file is the hardware side the original driver found on the PC:
// the PIT that clocked it, the OPL ports, the memory it read the banks from, and the audio output.
//
// Clock. The original programmed PIT channel 0 to 1000h counts, 291.304 Hz. Here the driver tick is derived from the
// audio sample clock (49716 Hz, the OPL2's own rate), sample-accurately: every output frame adds 1193182 to a phase
// accumulator and a PIT tick falls due each 4096 * 49716 units, before the frame at which it is due. The same function
// clocks the driver in every mode:
//   - window with an audio device: the SDL audio callback (audio thread, or the browser's audio callback on the web)
//     renders frames, so the music and the countdowns run while the game computes, as the PIT interrupt did. Game-side
//     driver calls take the audio lock. If the device stops calling back for 200 ms the event pump clocks the driver
//     from host time instead (no chip samples), so the countdown waits never hang.
//   - headless (and window without an audio device): the event pump clocks the driver up to port_time_us() (virtual
//     time headless: exactly 1 ms per poll, so runs are reproducible); chip samples are produced only when they are
//     written to --wav.
//
// Entry points: port_fm_function (the INT 66h subset of the SYSLIB helpers: countdowns, bank load) and port_fm_int66
// (MAIN's SOUND module, full registers). MAIN never uses the far-call entry or a client callback.
#include "fmdriver.hpp"
#include "hostinput.hpp"
#include "opl.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <bit>
#include <cinttypes>
#include <format>
#include <fstream>
#include <optional>
#include <print>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace opeu::services::detail::fm {
namespace {

constexpr auto PIT_UNITS_PER_FRAME{ fmdriver::PIT_HZ };
constexpr auto PIT_UNITS_PER_TICK{ fmdriver::PIT_DIVISOR * static_cast<std::uint32_t>(OPL_SAMPLE_RATE) };
constexpr auto STALL_US{ 200000u };
constexpr auto CHUNK_FRAMES{ 512uz }; // frames rendered at a time
constexpr auto SAMPLE_RATE{ static_cast<std::uint64_t>(OPL_SAMPLE_RATE) };

// `count` bytes of `value`, least significant first.
auto LittleEndian(std::uint32_t const value, int const count) -> std::string {
  return std::views::iota(0, count) | std::views::transform([value](int const i) { return static_cast<char>(value >> (8 * i)); }) | std::ranges::to<std::string>();
}

// The 44-byte RIFF header of 16-bit stereo PCM at the chip's rate.
auto WaveHeader(std::uint64_t const frames) -> std::string {
  auto const data{ static_cast<std::uint32_t>(frames * 4u) };
  auto const rate{ static_cast<std::uint32_t>(OPL_SAMPLE_RATE) };
  return "RIFF" + LittleEndian(data + 36, 4) + "WAVEfmt " + LittleEndian(16, 4) + LittleEndian(1, 2) /* PCM */ + LittleEndian(2, 2) /* stereo */
         + LittleEndian(rate, 4) + LittleEndian(rate * 4, 4) + LittleEndian(4, 2) /* block align */ + LittleEndian(16, 2) /* bits */ + "data" + LittleEndian(data, 4);
}

// The --wav output: the header, rewritten with the length when finished, then the samples as the host stores them.
class WaveFile {
public:
  [[nodiscard]] static auto Create(std::string const& path) -> std::optional<WaveFile> {
    std::ofstream stream{ path, std::ios::binary };
    if (!stream) return std::nullopt;
    stream << WaveHeader(0);
    return WaveFile{ std::move(stream) };
  }

  auto Append(std::span<std::int16_t const> const stereo) -> void {
    auto const bytes{ std::as_bytes(stereo) };
    _bytes.resize(bytes.size());
    std::ranges::transform(bytes, _bytes.begin(), [](std::byte const b) { return static_cast<char>(b); });
    _stream.write(_bytes.data(), static_cast<std::streamsize>(_bytes.size()));
    _frames += stereo.size() / 2;
  }

  auto Finish() -> void {
    _stream.seekp(0);
    _stream << WaveHeader(_frames);
    _stream.close();
  }

private:
  explicit WaveFile(std::ofstream stream) : _stream{ std::move(stream) } { }

  std::ofstream     _stream;
  std::uint64_t     _frames{};
  std::vector<char> _bytes; // the samples as the host stores them
};

// The driver's machine: the chip, the clock, the caller's memory and the recordings.
class Sound final : public FmHost {
public:
  auto               Init() -> void;
  auto               Clock() -> void;
  auto               Shutdown() -> void;
  [[nodiscard]] auto Interrupt(Request const& request) -> Reply;
  [[nodiscard]] auto Frames() const -> std::uint64_t {
    return _clock_frames;
  }

  auto               WriteRegister(std::uint8_t reg, std::uint8_t value) -> void override;
  [[nodiscard]] auto ReadStatus() -> std::uint8_t override;
  [[nodiscard]] auto Peek(FarAddress at) -> std::uint8_t override;
  [[nodiscard]] auto EndOfSong(FarAddress callback, SongQueue queue) -> SongQueue override;

private:
  auto OpenRecordings() -> void;
  auto OpenDevice() -> void;
  auto RenderFrames(std::span<std::int16_t> out, std::size_t frames) -> void;
  auto Advance(std::span<std::int16_t> out, std::size_t frames) -> void;
  auto ClockStalledDevice() -> void;

  Opl2                         _chip;
  FmDriver                     _driver{ *this };
  bool                         _installed{};
  bool                         _audio_device{};  // the audio callback clocks the driver
  std::uint32_t                _tick_phase{};    // PIT input clock units since the last tick
  std::uint64_t                _clock_frames{};  // output frames the driver clock has advanced
  std::uint64_t                _last_clock_us{}; // host time of the last callback / pump clocking
  std::optional<WaveFile>      _wav;
  std::optional<std::ofstream> _opl_log;
  std::vector<std::int16_t>    _scratch = std::vector<std::int16_t>(2 * CHUNK_FRAMES);
};

// ---- the driver's host ----

auto Sound::WriteRegister(std::uint8_t const reg, std::uint8_t const value) -> void {
  _chip.Write(reg, value);
  if (_opl_log) *_opl_log << std::format("{} {:02x} {:02x}\n", _clock_frames, reg, value);
}

auto Sound::ReadStatus() -> std::uint8_t {
  return 0;
}

// The banks are in the data arena (MAIN's FMUSIC at segment 3D91h, offsets 0 and 7BBAh).
auto Sound::Peek(FarAddress const at) -> std::uint8_t {
  auto const arena{
    std::span{ port_arena, PORT_ARENA_SIZE }
  };
  auto const linear{ (std::uint32_t{ at.segment } * 16u) + at.offset };
  return linear < arena.size() ? arena[linear] : std::uint8_t{ 0xFF };
}

auto Sound::EndOfSong(FarAddress const callback, SongQueue const queue) -> SongQueue {
  port_trace("fm", "client callback %04x:%04x ignored (MAIN installs none)", callback.segment, callback.offset);
  return queue;
}

// ---- the clock ----

// Chip samples to `out` (empty: not rendered, the chip only receives the register writes) and to the WAV file.
auto Sound::RenderFrames(std::span<std::int16_t> const out, std::size_t const frames) -> void {
  auto const target{ !out.empty() ? out.first(2 * frames) : _wav ? std::span{ _scratch }.first(2 * frames) : std::span<std::int16_t>{} };
  if (!target.empty()) _chip.Render(target);
  if (_wav) _wav->Append(target);
}

// Advance the driver clock by `frames` output frames. PIT ticks run at their exact frame positions.
auto Sound::Advance(std::span<std::int16_t> out, std::size_t frames) -> void {
  // A hand loop: each pass renders up to the next PIT tick (at most CHUNK_FRAMES) and runs the tick; the number of
  // passes follows from the tick phase.
  while (frames > 0) {
    auto const due{ _tick_phase >= PIT_UNITS_PER_TICK };
    auto const until{ due ? 0uz : (PIT_UNITS_PER_TICK - _tick_phase + PIT_UNITS_PER_FRAME - 1) / PIT_UNITS_PER_FRAME };
    auto const n{ std::min({ until, frames, CHUNK_FRAMES }) };
    if (n > 0) {
      RenderFrames(out, n);
      if (!out.empty()) out = out.subspan(2 * n);
      _tick_phase   += static_cast<std::uint32_t>(n) * PIT_UNITS_PER_FRAME;
      _clock_frames += n;
      frames        -= n;
    }
    if (_tick_phase >= PIT_UNITS_PER_TICK) {
      _tick_phase -= PIT_UNITS_PER_TICK;
      if (_installed) _driver.TimerTick();
    }
  }
}

// The device stopped pulling samples: keep the driver's time (at most 1 s per poll).
auto Sound::ClockStalledDevice() -> void {
  platform::AudioLock const lock;
  auto const                now{ platform::TicksUs() }; // after the lock: the callback may just have run
  if (now <= _last_clock_us || now - _last_clock_us <= STALL_US) return;
  auto const frames{ (now - _last_clock_us) * SAMPLE_RATE / 1000000u };
  Advance({}, static_cast<std::size_t>(std::min(frames, SAMPLE_RATE)));
  _last_clock_us = now;
}

// Called by every event pump.
auto Sound::Clock() -> void {
  if (!_installed) return;
  if (_audio_device) {
    ClockStalledDevice();
    return;
  }
  auto const target{ port_time_us() * SAMPLE_RATE / 1000000u };
  if (_clock_frames < target) Advance({}, static_cast<std::size_t>(target - _clock_frames));
}

auto Sound::Shutdown() -> void {
  platform::AudioLock const lock;
  if (_wav) {
    _wav->Finish();
    _wav.reset();
  }
  _opl_log.reset();
}

auto Sound::OpenRecordings() -> void {
  if (port_options.wav_path != nullptr) {
    _wav = WaveFile::Create(port_options.wav_path);
    if (!_wav) port_fatal("cannot write %s", port_options.wav_path);
  }
  if (port_options.opl_log_path != nullptr) {
    _opl_log.emplace(port_options.opl_log_path);
    if (!*_opl_log) port_fatal("cannot write %s", port_options.opl_log_path);
    *_opl_log << std::format("# frame (at {} Hz) register value\n", OPL_SAMPLE_RATE);
  }
}

auto Sound::OpenDevice() -> void {
  if (port_options.headless != 0 || port_options.mute != 0) return;
  _audio_device  = platform::OpenAudio(OPL_SAMPLE_RATE, [this](std::span<std::int16_t> const stereo) {
    Advance(stereo, stereo.size() / 2); // the SDL audio callback (the stream lock is held)
    _last_clock_us = platform::TicksUs();
  });
  _last_clock_us = platform::TicksUs();
  if (!_audio_device) std::println(stderr, "opeu: no audio output; the music is silent");
}

auto TheSound() -> Sound&;

// Install the driver (FMDRV ran from OPEU.BAT before MAIN) and the audio.
auto Sound::Init() -> void {
  OpenRecordings();
  _chip.Reset();
  // The emulated chip is always there; the timer-overflow detection of the original installer is not run.
  _driver.Install();
  _installed = true;
  std::atexit([] { TheSound().Shutdown(); });
  OpenDevice();
  port_trace("fm", "driver installed; clock: %s", _audio_device ? "audio device" : "event pump");
}

auto Sound::Interrupt(Request const& request) -> Reply {
  if (!_installed) return { .status = fmdriver::NO_DRIVER, .ticks = request.ticks, .address = request.address }; // no driver resident
  platform::AudioLock const lock;
  return _driver.Interrupt(request);
}

// The one sound machine of the process (legacy: the INT 66h entry points below are free functions).
auto TheSound() -> Sound& {
  static Sound sound;
  return sound;
}

// The callers busy-wait on the status functions and on the countdowns (a countdown read that returns 0 ends the wait:
// no poll).
auto Waits(Request const& request, Reply const& reply) -> bool {
  auto const countdown{ request.function == 0xF0 || request.function == 0xF1 };
  return request.function == 0x03 || request.function == 0x13 || (countdown && static_cast<std::uint8_t>(reply.status) == 0 && reply.ticks != 0);
}

}
}

void port_fm_clock() {
  opeu::services::detail::fm::TheSound().Clock();
}

void port_fm_init() {
  opeu::services::detail::fm::TheSound().Init();
}

// Full-register INT 66h (port_sound_int66 of the game's glue).
void port_fm_int66(uint16_t* ax, uint16_t* bx, uint16_t* dx, uint16_t* ds) {
  using namespace opeu::services::detail::fm;
  auto const request{
    opeu::services::Request{ .function = static_cast<std::uint8_t>(*ax >> 8), .argument = static_cast<std::uint8_t>(*ax), .ticks = *bx, .address = { .segment = *ds, .offset = *dx } }
  };
  auto const reply{ TheSound().Interrupt(request) };
  port_trace("fm", "frame %" PRIu64 " int66 ax=%04x bx=%04x dx=%04x ds=%04x -> ax=%04x bx=%04x", TheSound().Frames(), *ax, *bx, *dx, *ds, reply.status, reply.ticks);
  *ax = reply.status;
  *bx = reply.ticks;
  *dx = reply.address.offset;
  *ds = reply.address.segment;
  if (Waits(request, reply)) port_pump();
}

// INT 66h for the SYSLIB helpers: AH, AL, BX, DS:DX in; the countdown reads (F0h/F1h AL=0) return BX, everything else
// AX.
auto port_fm_function(uint8_t ah, uint8_t al, uint16_t bx, uint16_t dx_off, uint16_t ds_seg) -> uint16_t {
  auto ax{ static_cast<std::uint16_t>(ah << 8 | al) };
  auto dx{ dx_off };
  auto ds{ ds_seg };
  port_fm_int66(&ax, &bx, &dx, &ds);
  auto const countdown{ ah == 0xF0 || ah == 0xF1 };
  if (countdown && al == 0) return bx;
  return countdown ? 0 : ax;
}
