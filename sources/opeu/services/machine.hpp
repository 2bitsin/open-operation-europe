// The machine the game runs on: what the game asks of its display, files, input, sound and clock. Empty until M2's
// batch B5 moves the game's direct service calls behind it (Display, Files, Input, Sound, Clock); held by reference in
// opeu::game::Game, so a test can hand the game a fake machine.
#pragma once

namespace opeu::services::detail::machine {

class Machine {
public:
  Machine()                                  = default;
  Machine(Machine const&)                    = delete;
  Machine(Machine&&)                         = delete;
  auto operator=(Machine const&) -> Machine& = delete;
  auto operator=(Machine&&) -> Machine&      = delete;
  virtual ~Machine()                         = default;
};
} // namespace opeu::services::detail::machine

namespace opeu::services {
using detail::machine::Machine;
} // namespace opeu::services
