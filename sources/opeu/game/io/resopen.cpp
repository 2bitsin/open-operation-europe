// Opens the game's data files (read/write) and resets the DOS disk buffers.
#include "io/resopen.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/text.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// `name` is the DS offset of the file name. A failed open is followed by a close of the -1 handle, and -1 returned.
auto opeu_open_resource(Game& game, std::uint16_t name) -> std::int16_t {
  auto&      files{ game.Machine().Files() };
  auto const handle{ files.Open(CString(Data(game), name), services::OpenMode::READ_WRITE) };
  if (!handle) files.Close(HandleOf(NO_FILE));
  return StoredHandle(handle);
}

void opeu_disk_reset(Game& game) {
  game.Machine().Files().ResetDisk();
}
