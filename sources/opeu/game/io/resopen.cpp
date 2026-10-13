// Opens the game's data files (read/write) and resets the DOS disk buffers.
#include "io/resopen.hpp"

#include "common/game.hpp"
#include "common/memory.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t READ_WRITE{ 2 };
constexpr std::int16_t  FAILED{ -1 };
} // namespace

// `name` is the DS offset of the file name. A failed open is followed by a close of the -1 handle, and -1 returned.
auto opeu_open_resource(Game& game, std::uint16_t name) -> std::int16_t {
  auto const handle{ static_cast<std::int16_t>(static_cast<std::uint16_t>(opeu_dos_open(reinterpret_cast<char const*>(game.State().Bytes(name).data()), READ_WRITE))) };
  if (handle == FAILED) opeu_dos_close(static_cast<std::int16_t>(FAILED));
  return handle;
}

void opeu_disk_reset(Game& game) {
  port_disk_reset();
}
