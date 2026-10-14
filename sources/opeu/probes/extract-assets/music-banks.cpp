#include "music-banks.hpp"

#include "opeu/game/data/fmusic.hpp"
#include "opeu/intro/recovered/data/song-bank.hpp"
#include "opeu/services/fmdriver.hpp"

#include <algorithm>
#include <ranges>

namespace opeu::probes::extract_assets::detail::music_banks {

namespace {

constexpr auto BANK{
  services::FarAddress{ .segment = 0x1000, .offset = 0 }
};                                       // anywhere: the driver reads the bank where it is told
constexpr auto SONG_TICKS{ 30uz * 291 }; // the driver ticks at 291.3 Hz
constexpr auto EFFECT_TICKS{ 2uz * 291 };
constexpr auto LOAD_SONGS{ std::uint8_t{ 0x00 } };
constexpr auto PLAY_SONG{ std::uint8_t{ 0x01 } };
constexpr auto SET_REPEAT{ std::uint8_t{ 0x04 } };
constexpr auto LOAD_EFFECTS{ std::uint8_t{ 0x10 } };
constexpr auto PLAY_EFFECT{ std::uint8_t{ 0x11 } };

template <class _Words>
auto WordBytes(_Words const& words) -> std::vector<std::byte> {
  return words | std::views::join
         | std::views::transform([](auto const word) { return std::array{ std::byte{ static_cast<std::uint8_t>(word & 0xFF) }, std::byte{ static_cast<std::uint8_t>(word >> 8) } }; })
         | std::views::join | std::ranges::to<std::vector>();
}

auto AsBytes(std::span<std::uint8_t const> const bytes) -> std::vector<std::byte> {
  return bytes | std::views::transform([](std::uint8_t const b) { return std::byte{ b }; }) | std::ranges::to<std::vector>();
}

auto Joined(std::array<std::vector<std::byte>, 3> const& parts) -> std::vector<std::byte> {
  return parts | std::views::join | std::ranges::to<std::vector>();
}

// The machine the driver plays on: the bank as the caller's memory, the register writes recorded.
class RecordingHost final : public services::FmHost {
public:
  explicit RecordingHost(std::span<std::byte const> const bank) : _bank{ bank } { }

  auto WriteRegister(std::uint8_t const reg, std::uint8_t const value) -> void override {
    _writes.push_back((std::uint32_t{ reg } << 8) | value);
  }
  [[nodiscard]] auto ReadStatus() -> std::uint8_t override {
    return 0;
  }
  [[nodiscard]] auto Peek(services::FarAddress const at) -> std::uint8_t override {
    return at.offset < _bank.size() ? std::to_integer<std::uint8_t>(_bank[at.offset]) : 0;
  }
  [[nodiscard]] auto EndOfSong([[maybe_unused]] services::FarAddress const callback, services::SongQueue const queue) -> services::SongQueue override {
    return queue;
  }

  [[nodiscard]] auto Writes() const -> std::vector<std::uint32_t> const& {
    return _writes;
  }

private:
  std::span<std::byte const> _bank;
  std::vector<std::uint32_t> _writes;
};

auto Send(services::FmDriver& driver, std::uint8_t const function, std::uint8_t const argument) -> void {
  [[maybe_unused]] auto const reply{ driver.FarCall({ .function = function, .argument = argument, .ticks = 0, .address = BANK }) };
}

auto Tick(services::FmDriver& driver, std::size_t const ticks) -> void {
  std::ranges::for_each(std::views::iota(0uz, ticks), [&driver](std::size_t) { driver.TimerTick(); });
}

// Each entry of the bank played from its start, one after the other.
auto PlayEach(services::FmDriver& driver, assets::BankLayout const layout) -> void {
  auto const songs{ layout.kind == assets::BankKind::SONGS };
  Send(driver, songs ? LOAD_SONGS : LOAD_EFFECTS, 0);
  if (songs) Send(driver, SET_REPEAT, 1);
  std::ranges::for_each(std::views::iota(1uz, layout.entries + 1), [&](std::size_t const entry) {
    Send(driver, songs ? PLAY_SONG : PLAY_EFFECT, static_cast<std::uint8_t>(entry));
    Tick(driver, songs ? SONG_TICKS : EFFECT_TICKS);
  });
}

}

auto ShippedBanks() -> std::array<std::vector<std::byte>, 3> {
  namespace main_bank    = game::detail::fmusic;
  namespace opening_bank = intro::detail::song_bank;
  return { Joined({ WordBytes(main_bank::MUSIC_TRACKS), AsBytes(main_bank::MUSIC_VOICES), AsBytes(main_bank::MUSIC_EVENTS) }), AsBytes(main_bank::SOUND_EFFECTS),
           Joined({ WordBytes(opening_bank::TRACKS), AsBytes(opening_bank::VOICES), AsBytes(opening_bank::EVENTS) }) };
}

auto RegisterStream(std::span<std::byte const> const bank, assets::BankLayout const layout) -> std::vector<std::uint32_t> {
  auto host{ RecordingHost{ bank } };
  auto driver{ services::FmDriver{ host } };
  driver.Install();
  PlayEach(driver, layout);
  return host.Writes();
}

}
