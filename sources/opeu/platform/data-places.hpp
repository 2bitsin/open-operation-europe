#pragma once
// What the host tells the entry about the game's data before the game runs: the directories it may lie in (after
// --data and the files compiled in, docs/building.md "The game's data") and how the player is told it is missing.

#include <filesystem>
#include <string_view>
#include <vector>

namespace opeu::platform::detail::data_places {

// The host's own places for the 26 data files, in the order they are searched: on a desktop the executable's directory
// and the per-user data directory (the preference path's data/); in the browser /data, which the page fills.
[[nodiscard]] auto DataDirectories() -> std::vector<std::filesystem::path>;

// Tells the player in a windowed run that the game's data is missing (`report`: where it looked and what it lacks): a
// message box on a desktop; in the browser the page's file picker, which stores the files in /data and starts the game
// again.
auto ShowMissingData(std::string_view report) -> void;

}

namespace opeu::platform {
using detail::data_places::DataDirectories;
using detail::data_places::ShowMissingData;
}
