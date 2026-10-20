// Operation Europe, native: the application. The game's data files compiled in (embedded.hpp) are handed to the
// services' file layer; the entry is then opeu_main (entry.cpp, beside this file), crt0's role for the recovered MAIN.
#include "embedded.hpp"

#include "opeu/assets/game-data.hpp"
#include "opeu/services/asset-table.hpp"

#include <concepts>
#include <exception>
#include <print>
#include <string_view>

auto opeu_main(int argc, char** argv, opeu::assets::GameData const& data) -> int; // NOLINT(readability-identifier-naming): entry.cpp's, renamed with #17

namespace {
// The game's data handed to the file layer, then `start` on it.
auto Run(std::invocable<opeu::assets::GameData const&> auto const& start) -> int {
  auto table{ opeu::app::EmbeddedData() };
  if (!table) {
    std::println(stderr, "opeu: built without the game's data files; this build cannot run the game yet");
    return 3;
  }
  auto const data{ opeu::assets::GameData::FromTable(*std::move(table)) };
  opeu::services::FilesUseAssets([&data](std::string_view const name) { return data.DosFile(name); });
  return start(data);
}

// An exception that reached main, said on stderr; the exit code.
auto Failed(std::exception const& failure) noexcept -> int {
  try {
    std::println(stderr, "opeu: {}", failure.what());
    return 1;
  } catch (...) {
    return 1; // nothing is left to report to
  }
}
}

auto main(int argc, char** argv) -> int {
  try {
    return Run([argc, argv](opeu::assets::GameData const& data) { return opeu_main(argc, argv, data); });
  } catch (std::exception const& failure) {
    return Failed(failure);
  }
}
