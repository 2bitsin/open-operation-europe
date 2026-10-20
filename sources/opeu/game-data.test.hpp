#pragma once
// The game's data in the tests: the 26 data files, the classic pack, and the goldens, fixtures and samples made from
// them. The private repository holds all of them, so nothing skips there. A clone without them (the public mirror)
// skips exactly the tests that need them, each with "no game data: <what>", unless OPEU_DATA_DIR names a directory with
// the player's own 26 files; such a directory must be complete, or the tests fail. No test skips any other way. Tests
// run in their module's directory.

#include "opeu/assets/catalogue-pack.hpp"
#include "opeu/assets/catalogue.hpp"
#include "opeu/assets/data-files.hpp"
#include "opeu/assets/game-data.hpp"
#include "opeu/assets/pack.hpp"

#include <gtest/gtest.h>

#include <expected>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace opeu::detail::game_data_test {
using namespace std::string_view_literals;

inline constexpr auto NO_GAME_DATA{ "no game data: "sv };
inline constexpr auto DATA_DIRECTORY_VARIABLE{ "OPEU_DATA_DIR" };
inline constexpr auto IN_TREE_DATA{ "../opeu/assets.embed"sv };
inline constexpr auto IN_TREE_CLASSIC_PACK{ "../assets/packs.embed/classic"sv };

// The one skip message: what the test needs that this clone does not hold.
[[nodiscard]] inline auto NoGameData(std::string_view const what) -> std::string {
  return std::format("{}{}", NO_GAME_DATA, what);
}

// A file of the private repository made from the game's data (a golden, a fixture, a sample), if this clone holds it.
[[nodiscard]] inline auto TestFile(std::filesystem::path path) -> std::optional<std::filesystem::path> {
  if (!std::filesystem::exists(path)) return std::nullopt;
  return path;
}

// The directory OPEU_DATA_DIR names, if it is set.
[[nodiscard]] inline auto NamedDataDirectory() -> std::optional<std::filesystem::path> {
  auto const* const value{ ::testing::internal::posix::GetEnv(DATA_DIRECTORY_VARIABLE) }; // gtest's getenv, read before any test thread
  if (value == nullptr || *value == '\0') return std::nullopt;
  return std::filesystem::path{ value };
}

// The game's data a test reads: the `named` directory's (OPEU_DATA_DIR), else the `in_tree` one's; nothing when neither
// is given and the clone has none; why, when the directory found does not hold the release's files.
[[nodiscard]] inline auto FindTestData(std::optional<std::filesystem::path> const& named, std::filesystem::path const& in_tree) -> std::expected<std::optional<assets::GameData>, std::string> {
  auto const directory{ named.or_else([&in_tree] { return TestFile(in_tree); }) };
  if (!directory) return std::optional<assets::GameData>{};
  auto data{ assets::GameData::FromDirectory(*directory, assets::DATA_FILES) };
  if (!data) return std::unexpected{ data.error().Text() };
  return std::optional<assets::GameData>{ *std::move(data) };
}

// A test of the game's data: skipped without it, failed when the data found is not the release's.
class GameDataTest : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    auto found{ FindTestData(NamedDataDirectory(), IN_TREE_DATA) };
    if (!found) FAIL() << found.error();
    if (!found->has_value()) GTEST_SKIP() << NoGameData("the 26 data files (set OPEU_DATA_DIR to a directory holding them)");
    _data.emplace(*std::move(*found));
  }

  [[nodiscard]] auto Data() const -> assets::GameData const& {
    return _data.value();
  }

private:
  std::optional<assets::GameData> _data;
};

// A test of the classic pack: the private repository's checked-in pack, else the decoders' pack over the game's data
// (the same images, entries, palettes, animations and fonts), else skipped.
class ClassicPackTest : public GameDataTest {
protected:
  auto SetUp() -> void override {
    if (auto const checked_in{ TestFile(IN_TREE_CLASSIC_PACK) }) {
      _pack = std::make_unique<assets::Pack const>(assets::OpenPack(*checked_in).value());
      return;
    }
    GameDataTest::SetUp();
    if (IsSkipped() || HasFailure()) return;
    _pack = std::make_unique<assets::CataloguePack const>(assets::Catalogue{ Data() });
  }

  [[nodiscard]] auto Classic() const -> assets::AssetPack const& {
    return *_pack;
  }

private:
  std::unique_ptr<assets::AssetPack const> _pack;
};

}

namespace opeu::test {
using detail::game_data_test::ClassicPackTest;
using detail::game_data_test::FindTestData;
using detail::game_data_test::GameDataTest;
using detail::game_data_test::IN_TREE_CLASSIC_PACK;
using detail::game_data_test::IN_TREE_DATA;
using detail::game_data_test::NO_GAME_DATA;
using detail::game_data_test::NoGameData;
using detail::game_data_test::TestFile;
}
