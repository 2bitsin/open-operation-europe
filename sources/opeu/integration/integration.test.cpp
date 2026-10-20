// The fixtures the integration suites replay: the scripted inputs of the headless smoke runs, and the saved game in the
// original format (made with the game's data, so the private repository's only: a clone without it skips that case).
#include "opeu/game-data.test.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>

namespace {

// ctest runs the module's tests from the module directory.
auto FixtureSize(char const* name) -> std::uintmax_t {
  auto const path{ std::filesystem::path{ "fixtures" } / name };
  return std::filesystem::is_regular_file(path) ? std::filesystem::file_size(path) : 0;
}

TEST(Fixtures, HoldTheSmokeScripts) {
  EXPECT_GT(FixtureSize("smoke-input.txt"), 0U);
  EXPECT_GT(FixtureSize("smoke-turn.txt"), 0U);
}

class SavedGameFixture : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    if (!::opeu::test::TestFile("fixtures/OPEU.SAV")) GTEST_SKIP() << ::opeu::test::NoGameData("the saved game in the original format (fixtures/OPEU.SAV)");
  }
};

TEST_F(SavedGameFixture, IsNotEmpty) {
  EXPECT_GT(FixtureSize("OPEU.SAV"), 0U);
}

} // namespace
