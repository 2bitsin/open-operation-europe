// The opening's state: OPEN.EXE's data segment (DGROUP) as typed records, and the stack that grows down from its top
// (SS = DS). The segment is viewed where OPEN's memory holds it, as GameState views MAIN's.
#pragma once

#include "dgroup.hpp"
#include "segment-bytes.hpp"

#include "opeu/contract.hpp"

#include <cstdint>

namespace opeu::intro::detail::open_state {

inline constexpr std::uint16_t OPEN_DS_END{ sizeof(OpenDataSegment) }; // _end: past DGROUP's data and BSS
inline constexpr std::uint16_t OPEN_STACK_TOP{ 0x3040 };               // SP after the C start-up

class OpenState {
public:
  explicit OpenState(Segment segment) noexcept : _segment{ segment } { }

  [[nodiscard]] auto Data() noexcept -> OpenDataSegment& {
    return *reinterpret_cast<OpenDataSegment*>(_segment.data()); // NOLINT(*-reinterpret-cast): DGROUP's bytes as its records
  }
  [[nodiscard]] auto Data() const noexcept -> OpenDataSegment const& {
    return *reinterpret_cast<OpenDataSegment const*>(_segment.data()); // NOLINT(*-reinterpret-cast): DGROUP's bytes as its records
  }
  // Every byte of the segment, addressed by near pointer.
  [[nodiscard]] auto Bytes() const noexcept -> Segment {
    return _segment;
  }
  [[nodiscard]] auto StackPointer() const noexcept -> std::uint16_t {
    return _stack_pointer;
  }
  // Pushes `bytes` onto the stack; the near address of the bytes pushed.
  auto Reserve(std::uint16_t bytes) -> std::uint16_t {
    Expects(_stack_pointer >= OPEN_DS_END + bytes, "the stack stays above DGROUP's data");
    _stack_pointer = static_cast<std::uint16_t>(_stack_pointer - bytes);
    return _stack_pointer;
  }
  auto Release(std::uint16_t bytes) noexcept -> void {
    _stack_pointer = static_cast<std::uint16_t>(_stack_pointer + bytes);
  }

private:
  Segment       _segment;
  std::uint16_t _stack_pointer{ OPEN_STACK_TOP };
};

// Bytes on the stack for as long as the reservation lives: a routine's buffer, a return address or the words a caller
// pushes.
class StackReservation {
public:
  StackReservation(OpenState& state, std::uint16_t bytes) : _state{ state }, _bytes{ bytes }, _near{ state.Reserve(bytes) } { }
  StackReservation(StackReservation const&)                    = delete;
  StackReservation(StackReservation&&)                         = delete;
  auto operator=(StackReservation const&) -> StackReservation& = delete;
  auto operator=(StackReservation&&) -> StackReservation&      = delete;
  ~StackReservation() {
    _state.Release(_bytes);
  }

  [[nodiscard]] auto Near() const noexcept -> std::uint16_t {
    return _near;
  }

private:
  OpenState&    _state;
  std::uint16_t _bytes;
  std::uint16_t _near;
};
} // namespace opeu::intro::detail::open_state

namespace opeu::intro {
using detail::open_state::OPEN_DS_END;
using detail::open_state::OPEN_STACK_TOP;
using detail::open_state::OpenState;
using detail::open_state::StackReservation;
} // namespace opeu::intro
