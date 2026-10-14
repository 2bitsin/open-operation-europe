// The OPL2 chip: ymfm::ym3812 with the key retriggers a real chip plays. ymfm is vendored (one token changed, see
// docs/port.md); this is emulation of the sound chip (a hardware service behind the platform layer), not of the CPU.
#include "opl.hpp"

#include <algorithm>
#include <ranges>

namespace opeu::services::detail::opl {
namespace {

// At most this many samples are generated ahead by retriggers; more are generated and dropped.
constexpr auto QUEUE_LIMIT{ 64uz };

// Output gain: ymfm's YM3812 output for one full-level voice peaks near 4000 of 32767; the game's songs stay below
// about 5000 with all voices. x3 brings them to a normal listening level and keeps headroom.
constexpr std::int32_t OUTPUT_GAIN{ 3 };

}

Opl2::Opl2() : _chip{ *this } {
  _chip.reset();
}

auto Opl2::Reset() -> void {
  _chip.reset();
  _key_regs   = {};
  _rhythm_reg = 0;
  _released   = 0;
  _queued.clear();
}

auto Opl2::NextSample() -> std::int16_t {
  ymfm::ym3812::output_data out;
  _chip.generate(&out, 1);
  _released = 0;
  return static_cast<std::int16_t>(out.data[0]);
}

// Key retriggers. ymfm latches a key-on bit when it is written and compares it with the previous state once per output
// sample, so a key released and struck again between two samples never retriggers. The FM driver does exactly that
// within one timer tick (the opening's snare march writes BDh 20h then 28h), and a real chip or DOSBox's OPL, which
// act on each write, play every stroke. When a key released since the last sample is struck again, the chip is
// advanced by one sample first, as the microseconds between the writes would on hardware; that sample is queued and
// handed out by the next Render, so the output keeps its length.
auto Opl2::RetriggerGap(std::uint32_t const struck) -> void {
  if ((struck & _released) == 0) return;
  auto const sample{ NextSample() };
  if (_queued.size() < QUEUE_LIMIT) _queued.push_back(sample);
}

auto Opl2::Write(std::uint8_t const reg, std::uint8_t const value) -> void {
  if (reg >= 0xB0 && reg <= 0xB8) {
    auto const channel{ static_cast<std::size_t>(reg - 0xB0) };
    auto const bit{ 1u << channel };
    auto const was{ (_key_regs[channel] & 0x20) != 0 };
    auto const now{ (value & 0x20) != 0 };
    if (!was && now) {
      RetriggerGap(bit);
    } else if (was && !now) {
      _released |= bit;
    }
    _key_regs[channel] = value;
  } else if (reg == 0xBD) {
    auto const was{ _rhythm_reg & 0x1Fu };
    auto const now{ value & 0x1Fu };
    if ((value & 0x20) != 0) RetriggerGap((now & ~was) << 9);
    _released   |= (was & ~now) << 9;
    _rhythm_reg  = value;
  }
  _chip.write(0, reg);
  _chip.write(1, value);
}

auto Opl2::Render(std::span<std::int16_t> const stereo) -> void {
  std::ranges::for_each(std::views::iota(0uz, stereo.size() / 2), [&](std::size_t const frame) {
    auto const queued{ !_queued.empty() };
    auto const sample{ queued ? _queued.front() : NextSample() };
    if (queued) _queued.pop_front();
    auto const loud{ static_cast<std::int16_t>(std::clamp(sample * OUTPUT_GAIN, std::int32_t{ -32768 }, std::int32_t{ 32767 })) };
    stereo[2 * frame]       = loud;
    stereo[(2 * frame) + 1] = loud;
  });
}

}
