// Loads a scenario's record and its map from the contents of the scenario and map files.
#pragma once

#include "common/events.hpp"
#include "common/scenario.hpp"

#include <cstdint>
#include <expected>
#include <span>

namespace opeu::game::detail::loadscen {

// Why a scenario did not load; the data segment is untouched then.
enum class LoadError : std::uint8_t {
  NO_SUCH_SCENARIO,
  SCENARIO_FILE_SHORT, // the scenario file ends before the scenario's record
  MAP_FILE_SHORT,      // the map file ends before the record's map
};

// The contents of the scenario file (SCEDATA) and the map file (MAPINF).
struct ScenarioFiles {
  std::span<std::uint8_t const> scenarios;
  std::span<std::uint8_t const> maps;
};

// Copies the scenario's record into the data segment, rebased, then the map its scenario byte names.
auto LoadScenario(Game& game, Scenario scenario, ScenarioFiles files) -> std::expected<void, LoadError>;

// Reads the two files the data segment names (scenario_filename, map_filename) and loads the scenario from them.
auto LoadScenarioFromFiles(Game& game, Scenario scenario) -> std::expected<void, LoadError>;

} // namespace opeu::game::detail::loadscen

namespace opeu::game {
using detail::loadscen::LoadError;
using detail::loadscen::LoadScenario;
using detail::loadscen::LoadScenarioFromFiles;
using detail::loadscen::ScenarioFiles;
} // namespace opeu::game
