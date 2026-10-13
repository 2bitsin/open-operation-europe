// The machine: the native one owns its adapters; an injected one only refers to its devices.
#include "machine.hpp"

#include "native-machine.hpp"

namespace opeu::services::detail::machine {

Machine::Machine() : _native{ std::make_unique<NativeDevices>() }, _devices{ _native->Devices() } { }

Machine::Machine(Devices const devices) noexcept : _devices{ devices } { }

Machine::~Machine() = default;

}
