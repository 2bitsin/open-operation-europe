// Hourly wound recovery of the generals, and the death of a general replaced by a deputy.
#pragma once

#include "common/events.hpp"
#include "common/text.hpp"
#include "types.hpp"

#include <cstddef>
#include <cstdint>

namespace opeu::game::detail::evwound {

// The death notice the adviser shows, "Commander " + name + suffix, with the general's side after the text's start as
// the original kept it in its frame: at byte 50 of a 52-byte frame. A name of seven or more characters runs the text
// into the side byte, and the second status test then reads the overwritten side; a name of nine or more ran the text
// past the frame into the caller's, whose room the buffer's tail stands in for.
class DeathNotice {
public:
  static constexpr std::size_t SIDE_AT{ 50 };
  static constexpr std::size_t BYTES{ 0x80 };

  explicit DeathNotice(std::uint8_t side) noexcept {
    _message.Characters()[SIDE_AT] = static_cast<char>(side);
  }

  // The side byte, as the status tests read it.
  [[nodiscard]] auto Side() noexcept -> std::uint8_t {
    return static_cast<std::uint8_t>(_message.Characters()[SIDE_AT]);
  }
  [[nodiscard]] auto Message() noexcept -> TextBuffer<BYTES>& {
    return _message;
  }

private:
  TextBuffer<BYTES> _message;
};

// The general's death notice: his side, and the message when a player commands that side.
[[nodiscard]] auto ComposeDeathNotice(Game& game, std::uint16_t person) -> DeathNotice;
} // namespace opeu::game::detail::evwound

namespace opeu::game {
using detail::evwound::ComposeDeathNotice;
using detail::evwound::DeathNotice;
} // namespace opeu::game

void opeu_death_present(Game& game);
void opeu_commander_death(Game& game, uint16_t person);
void opeu_wound_event(Game& game);
