// What every game routine receives: the game's state and the machine it runs on. One Game, built by the entry over the
// state it starts (opeu/entry.cpp), is handed down by reference; a test builds its own over a state and a fake machine.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/game-state.hpp"
#include "common/host.hpp"

#include "machine.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::game::detail::game {

class Game {
public:
  // A game whose UI primitives reach the glue's far-data arena.
  Game(GameState& state, services::Machine& machine) : Game{ state, machine, ArenaUi() } { }
  Game(GameState& state, services::Machine& machine, UiHost ui) : _segment{ state.Bytes(0, SEGMENT_SIZE) }, _state{ state }, _machine{ machine }, _ui{ ui } { }

  [[nodiscard]] auto State() noexcept -> GameState& {
    return _state;
  }
  [[nodiscard]] auto State() const noexcept -> GameState const& {
    return _state;
  }
  [[nodiscard]] auto Machine() noexcept -> services::Machine& {
    return _machine;
  }

  // The far memory and segment numbers of the UI primitives, and the dialogs' load segment and buffer area.
  [[nodiscard]] auto Ui() const noexcept -> UiHost const& {
    return _ui;
  }
  [[nodiscard]] auto Dialogs() const noexcept -> DialogHost const& {
    return _dialogs;
  }

  // The state's data segment, the bytes behind every Data, Byte and Word: one load away from the game, since the
  // routines reach it on nearly every statement.
  [[nodiscard]] auto Segment() const noexcept -> SegmentBytes {
    return _segment;
  }

private:
  SegmentBytes       _segment;
  GameState&         _state;
  services::Machine& _machine;
  UiHost             _ui;
  DialogHost         _dialogs;
};

// The data segment's typed records, the game's way to its fields.
[[nodiscard]] inline auto Data(Game& game) noexcept -> DataSegment& {
  return data_segment::Data(game.Segment().data());
}
[[nodiscard]] inline auto Data(Game const& game) noexcept -> DataSegment const& {
  return data_segment::Data(static_cast<std::uint8_t const*>(game.Segment().data()));
}

// The far memory from segment:offset to its end: an image or a buffer a drawing service reads or writes, whose extent
// only the service knows (B7 gives the far data typed buffers). A far address inside the data segment reaches the
// state's segment, up to its end.
[[nodiscard]] inline auto FarBytes(Game const& game, std::uint16_t segment, std::uint16_t offset) -> std::span<std::uint8_t> {
  auto const linear{ std::size_t{ segment } * 16 + offset };
  auto const data_segment{ std::size_t{ game.Ui().ds_segment } * 16 };
  if (linear >= data_segment && linear - data_segment < SEGMENT_SIZE) return std::span{ game.Segment() }.subspan(linear - data_segment);
  Expects(linear < game.Ui().memory.size(), "a far address lies inside far memory");
  return game.Ui().memory.subspan(linear);
}

// The byte at a near address, as a reference: a byte variable a routine receives by its near address.
[[nodiscard]] inline auto Byte(Game& game, std::uint16_t near) noexcept -> std::uint8_t& {
  return game.Segment()[near];
}
[[nodiscard]] inline auto Byte(Game const& game, std::uint16_t near) noexcept -> std::uint8_t const& {
  return game.Segment()[near];
}

// The word at a near address: little-endian, the address of its high byte wraps at 16 bits.
[[nodiscard]] inline auto Word(Game const& game, std::uint16_t near) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(Byte(game, near) | Byte(game, static_cast<std::uint16_t>(near + 1)) << 8);
}

// Stores a word at a near address; returns the value stored.
inline auto SetWord(Game& game, std::uint16_t near, std::uint16_t value) noexcept -> std::uint16_t {
  Byte(game, near)                                 = static_cast<std::uint8_t>(value);
  Byte(game, static_cast<std::uint16_t>(near + 1)) = static_cast<std::uint8_t>(value >> 8);
  return value;
}
} // namespace opeu::game::detail::game

namespace opeu::game {
namespace services = opeu::services; // the machine's interfaces and value types, for the routines outside opeu::game
using detail::game::Byte;
using detail::game::Data;
using detail::game::FarBytes;
using detail::game::Game;
using detail::game::SetWord;
using detail::game::Word;
} // namespace opeu::game
