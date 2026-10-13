// What every routine of the opening receives: its state, its memory and the machine it runs on. The runner builds one
// Open over the memory it loads (run.cpp) and hands it down by reference; a test builds its own over its own bytes and
// a fake machine.
#pragma once

#include "dgroup.hpp"
#include "open-memory.hpp"
#include "open-state.hpp"
#include "segment-bytes.hpp"

#include "opeu/services/machine.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace opeu::intro::detail::open {

class Open {
public:
  Open(OpenState& state, OpenMemory& memory, services::Machine& machine) noexcept : _state{ state }, _memory{ memory }, _machine{ machine } { }

  [[nodiscard]] auto State() noexcept -> OpenState& {
    return _state;
  }
  [[nodiscard]] auto State() const noexcept -> OpenState const& {
    return _state;
  }
  // Every paragraph OPEN addresses: its far buffers, the song bank, its DOS blocks.
  [[nodiscard]] auto Memory() noexcept -> OpenMemory& {
    return _memory;
  }
  [[nodiscard]] auto Machine() noexcept -> services::Machine& {
    return _machine;
  }
  // DGROUP's bytes, behind every Byte and Word.
  [[nodiscard]] auto Segment() const noexcept -> intro::Segment {
    return _state.Bytes();
  }

private:
  OpenState&         _state;
  OpenMemory&        _memory;
  services::Machine& _machine;
};

// DGROUP's typed records, the opening's way to its fields.
[[nodiscard]] inline auto Data(Open& open) noexcept -> OpenDataSegment& {
  return open.State().Data();
}
[[nodiscard]] inline auto Data(Open const& open) noexcept -> OpenDataSegment const& {
  return open.State().Data();
}

// The byte at a near address, as a reference.
[[nodiscard]] inline auto Byte(Open const& open, std::uint16_t near) noexcept -> std::uint8_t& {
  return open.Segment()[near];
}

// The word at a near address: little-endian, the address of its high byte wraps at 16 bits.
[[nodiscard]] inline auto Word(Open const& open, std::uint16_t near) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(Byte(open, near) | Byte(open, static_cast<std::uint16_t>(near + 1)) << 8);
}

// Stores a word at a near address; returns the value stored.
inline auto SetWord(Open const& open, std::uint16_t near, std::uint16_t value) noexcept -> std::uint16_t {
  Byte(open, near)                                 = static_cast<std::uint8_t>(value);
  Byte(open, static_cast<std::uint16_t>(near + 1)) = static_cast<std::uint8_t>(value >> 8);
  return value;
}

// The near pointer (DS offset) of a byte array in DGROUP (a string, a palette), where the opening hands one on.
template <class _Byte, std::size_t SIZE>
  requires(sizeof(_Byte) == 1)
[[nodiscard]] inline auto Near(Open const& open, _Byte const (&object)[SIZE]) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(reinterpret_cast<std::byte const*>(object) - std::as_bytes(open.Segment()).data()); // NOLINT(*-reinterpret-cast): a field's offset in the segment
}
} // namespace opeu::intro::detail::open

namespace opeu::intro {
using detail::open::Byte;
using detail::open::Data;
using detail::open::Near;
using detail::open::Open;
using detail::open::SetWord;
using detail::open::Word;
} // namespace opeu::intro
