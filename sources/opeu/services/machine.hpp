// The machine the game runs on: its display, files, input, sound and clock, each behind an interface. Held by
// reference in opeu::game::Game (and opeu::intro::Open); a default-constructed machine is the native one (adapters over
// the services' functions, native-machine.hpp), a test hands the game its own devices, fakes for instance.
#pragma once

#include "machine-clock.hpp"
#include "machine-display.hpp"
#include "machine-files.hpp"
#include "machine-input.hpp"
#include "machine-sound.hpp"

#include <memory>

namespace opeu::services::detail::machine {

class NativeDevices;

// The devices a machine is made of; they outlive it.
struct Devices {
  services::Display& display;
  services::Files&   files;
  services::Input&   input;
  services::Sound&   sound;
  services::Clock&   clock;
};

class Machine {
public:
  // The native machine.
  Machine();
  explicit Machine(Devices devices) noexcept;
  Machine(Machine const&)                    = delete;
  Machine(Machine&&)                         = delete;
  auto operator=(Machine const&) -> Machine& = delete;
  auto operator=(Machine&&) -> Machine&      = delete;
  ~Machine();

  [[nodiscard]] auto Display() noexcept -> services::Display& {
    return _devices.display;
  }
  [[nodiscard]] auto Files() noexcept -> services::Files& {
    return _devices.files;
  }
  [[nodiscard]] auto Input() noexcept -> services::Input& {
    return _devices.input;
  }
  [[nodiscard]] auto Sound() noexcept -> services::Sound& {
    return _devices.sound;
  }
  [[nodiscard]] auto Clock() noexcept -> services::Clock& {
    return _devices.clock;
  }

private:
  std::unique_ptr<NativeDevices> _native; // the native machine's adapters; none when the devices are injected
  Devices                        _devices;
};

}

namespace opeu::services {
using detail::machine::Devices;
using detail::machine::Machine;
}
