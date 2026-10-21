// Where the run's 26 data files come from (docs/building.md "The game's data"): the directory --data names and nothing
// else; else the files compiled into the executable; else the first directory of the search that holds them all
// ($OPEU_DATA_DIR, the host's places, the current directory). A directory counts only when every file has the release's
// size and fingerprint (assets/data-files.hpp).
#pragma once

#include "opeu/assets/game-data.hpp"

#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace opeu::app::detail::data_choice {

// The places the command line and the host give.
struct DataPlaces {
  std::optional<std::filesystem::path> named;  // --data DIR: only there, with no fallback
  std::vector<std::filesystem::path>   search; // where to look when neither --data nor the build gives the files
};

// The search: the directory `environment` names ($OPEU_DATA_DIR) if any, the host's places, then the current directory.
[[nodiscard]] auto SearchPlaces(std::optional<std::string_view> environment, std::vector<std::filesystem::path> host) -> std::vector<std::filesystem::path>;

// The run's data: the named directory's, else `embedded`'s, else the search's first complete directory; the report of
// every directory tried when none holds the files.
[[nodiscard]] auto ChooseData(DataPlaces const& places, std::optional<assets::GameData::Table> embedded) -> std::expected<assets::GameData, assets::DataReport>;

// What --check-data says: the verdict on each file of the directory the run would read (or that the files are
// compiled in), and whether the run would start.
struct DataCheck {
  std::string text;
  bool        complete{};
};

[[nodiscard]] auto CheckData(DataPlaces const& places, std::optional<assets::GameData::Table> const& embedded) -> DataCheck;

}

namespace opeu::app {
using detail::data_choice::CheckData;
using detail::data_choice::ChooseData;
using detail::data_choice::DataCheck;
using detail::data_choice::DataPlaces;
using detail::data_choice::SearchPlaces;
}
