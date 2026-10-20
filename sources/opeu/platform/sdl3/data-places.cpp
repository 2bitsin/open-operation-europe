// The host's places for the game's data (data-places.hpp): the target's directories and its way of telling the player.
#include "opeu/platform/data-places.hpp"

#include "opeu/platform/sdl3/target.hpp"

namespace opeu::platform::detail::data_places {

auto DataDirectories() -> std::vector<std::filesystem::path> {
  return target::DataDirectories();
}

auto ShowMissingData(std::string_view const report) -> void {
  target::ShowMissingData(report);
}

}
