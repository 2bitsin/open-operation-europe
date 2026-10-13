// Opens the game's data files (read/write) and resets the DOS disk buffers.
#include "io/resopen.hpp"

#include "common/game.hpp"
#include "common/memory.hpp"
#include "link/glue.hpp"
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
auto opeu_open_resource(Game& game, HostEvent event, std::uint16_t name) -> std::int16_t {
  auto const handle{ static_cast<std::int16_t>(Call(game, event, OPEN, name, READ_WRITE)) };
  if (handle == FAILED) Call(game, event, CLOSE, FAILED);
  return handle;
}

void opeu_disk_reset(Game& game, HostEvent event) {
  Call(game, event, DISK_RESET);
}

auto svc_resopen(Game& game, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (id) {
  case OPEN:       return static_cast<std::uint16_t>(opeu_dos_open(reinterpret_cast<char const*>(game.State().Bytes(Arg(args, count, 0)).data()), Arg(args, count, 1)));
  case CLOSE:      return static_cast<std::uint16_t>(opeu_dos_close(static_cast<std::int16_t>(Arg(args, count, 0))));
  case DISK_RESET: port_disk_reset(); break;
  default:         port_unhandled("resopen", id, count);
  }
  return 0;
}
