// Loads a scenario: its record (the data segment from scenario_first through scenario_last) from the scenario file,
// rebased, then the map its scenario byte names from the map file.
#include "io/loadscen.hpp"

#include "common/data-segment.hpp"
#include "common/dos-file.hpp"
#include "common/game.hpp"
#include "common/scenario.hpp"
#include "io/rebase.hpp"
#include "io/resopen.hpp"

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace opeu::game;

namespace {

constexpr auto        SCENARIO_COUNT{ 7u };
constexpr std::size_t SCENARIOS_AT{ 0x2580 }; // the records' offset in the scenario file
constexpr std::size_t RECORD_SIZE{ offsetof(DataSegment, scenario_last) + 1 - offsetof(DataSegment, scenario_first) };
constexpr std::size_t MAP_SIZE{ sizeof(DataSegment::map_cells) };

using FileBytes = std::span<std::uint8_t const>;

constexpr std::size_t READ_CHUNK{ 0x8000 }; // the most one read asks for

// The bytes of a file from the position it is at up to `size`, read a chunk at a time; fewer when a read fails.
auto ReadAll(services::Files& files, services::FileHandle file, std::size_t size) -> std::vector<std::uint8_t> {
  std::vector<std::uint8_t> bytes(size);
  std::size_t               done{ 0 };
  while (done < size) {
    auto const read{ files.Read(file, std::span{ bytes }.subspan(done, std::min(size - done, READ_CHUNK))) };
    if (!read || *read == 0) break;
    done += *read;
  }
  bytes.resize(done);
  return bytes;
}

// The whole contents of the resource file the DS string at `name` names; none when it cannot be opened.
auto ResourceContents(Game& game, std::uint16_t name) -> std::vector<std::uint8_t> {
  auto const stored{ opeu_open_resource(game, name) };
  if (stored == NO_FILE) return {};
  auto&      files{ game.Machine().Files() };
  auto const file{ HandleOf(stored) };
  auto const end{ files.Seek(file, 0, services::SeekOrigin::END).value_or(0) };
  files.Seek(file, 0, services::SeekOrigin::START);
  auto bytes{ ReadAll(files, file, static_cast<std::size_t>(end)) };
  files.Close(file);
  return bytes;
}

// The `size` bytes at `position` of a file, when the file holds them.
auto Slice(FileBytes file, std::size_t position, std::size_t size) -> std::optional<FileBytes> {
  if (position > file.size() || file.size() - position < size) return std::nullopt;
  return file.subspan(position, size);
}
} // namespace

auto opeu::game::detail::loadscen::LoadScenario(Game& game, Scenario scenario, ScenarioFiles files) -> std::expected<void, LoadError> {
  auto const index{ std::to_underlying(scenario) };
  if (index >= SCENARIO_COUNT) return std::unexpected{ LoadError::NO_SUCH_SCENARIO };
  auto const record{ Slice(files.scenarios, SCENARIOS_AT + index * RECORD_SIZE, RECORD_SIZE) };
  if (!record) return std::unexpected{ LoadError::SCENARIO_FILE_SHORT };
  // the record's second byte is its scenario byte
  auto const map{ Slice(files.maps, ((*record)[1] & SCENARIO_MASK) * MAP_SIZE, MAP_SIZE) };
  if (!map) return std::unexpected{ LoadError::MAP_FILE_SHORT };
  auto& data{ Data(game) };
  std::ranges::copy(*record, game.State().Bytes(offsetof(DataSegment, scenario_first), RECORD_SIZE).begin());
  opeu_rebase_scenario(game, 0);
  std::ranges::copy(files.maps.subspan(std::to_underlying(CurrentScenario(data)) * MAP_SIZE, MAP_SIZE), data.map_cells.begin());
  return {};
}

auto opeu::game::detail::loadscen::LoadScenarioFromFiles(Game& game, Scenario scenario) -> std::expected<void, LoadError> {
  auto const& data{ Data(game) };
  auto const  scenarios{ ResourceContents(game, Near(data, data.scenario_filename)) };
  auto const  maps{ ResourceContents(game, Near(data, data.map_filename)) };
  return LoadScenario(game, scenario, { .scenarios = scenarios, .maps = maps });
}
