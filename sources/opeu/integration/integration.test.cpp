// The fixtures the integration suites replay: the scripted inputs of the
// headless smoke runs and the saved game in the original format.
#include <gtest/gtest.h>

#include <filesystem>

namespace {

// ctest runs the module's tests from the module directory.
auto FixtureSize(char const* name) -> std::uintmax_t {
  auto const path{ std::filesystem::path{ "fixtures" } / name };
  return std::filesystem::is_regular_file(path) ? std::filesystem::file_size(path) : 0;
}

TEST(Fixtures, HoldTheSmokeScriptsAndTheSavedGame) {
  EXPECT_GT(FixtureSize("smoke-input.txt"), 0U);
  EXPECT_GT(FixtureSize("smoke-turn.txt"), 0U);
  EXPECT_GT(FixtureSize("OPEU.SAV"), 0U);
}

}  // namespace
