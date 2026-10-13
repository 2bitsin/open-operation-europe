// Opens the game's data files (read/write) and resets the DOS disk buffers.
#include "io/resopen.hpp"

#include "common/memory.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  OPEN       = 0, // (name, mode) -> handle
  CLOSE      = 1, // (handle)
  DISK_RESET = 2, // resets the DOS disk buffers
};

constexpr std::uint16_t READ_WRITE{ 2 };
constexpr std::int16_t  FAILED{ -1 };
} // namespace

// `name` is the DS offset of the file name. A failed open is followed by a close of the -1 handle, and -1 returned.
auto opeu_open_resource(std::uint8_t* ds, HostEvent event, std::uint16_t name) -> std::int16_t {
  auto const handle{ static_cast<std::int16_t>(Call(ds, event, OPEN, name, READ_WRITE)) };
  if (handle == FAILED) Call(ds, event, CLOSE, FAILED);
  return handle;
}

void opeu_disk_reset(std::uint8_t* ds, HostEvent event) {
  Call(ds, event, DISK_RESET);
}

auto svc_resopen(std::uint8_t* ds, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case OPEN:       return static_cast<std::uint16_t>(opeu_dos_open(reinterpret_cast<char const*>(ds + Arg(args, count, 0)), Arg(args, count, 1)));
  case CLOSE:      return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  case DISK_RESET: port_disk_reset(); break;
  default:         port_unhandled("resopen", id, count);
  }
  return 0;
}
