// The game's data from a directory: found in any case, checked by size and fingerprint, refused with a report that names
// what is wrong; and the compiled table of the 26 files against the shipped files.
#include "shipped.hpp"

#include "opeu/assets/fingerprint.hpp"
#include "opeu/assets/game-data.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::assets {
namespace {

using namespace std::string_view_literals;

[[nodiscard]] auto BytesOf(std::string_view const text) -> std::vector<std::byte> {
  return text | std::views::transform([](char const c) { return static_cast<std::byte>(c); }) | std::ranges::to<std::vector>();
}

[[nodiscard]] auto Expected(std::string_view const name, std::string_view const content) -> DataFile {
  return DataFile{ .name = name, .size = content.size(), .fingerprint = FingerprintOf(BytesOf(content)) };
}

inline constexpr auto ONE_CONTENT{ "the first file"sv };
inline constexpr auto TWO_CONTENT{ "and the second, a little longer"sv };

// Two synthetic files stand for the release's 26.
[[nodiscard]] auto TwoFiles() -> std::array<DataFile, 2> {
  return { Expected("ONE.DAT", ONE_CONTENT), Expected("TWO", TWO_CONTENT) };
}

// A fresh directory under the test's temporary directory, holding `files` (name, content).
class Directory {
public:
  explicit Directory(std::string_view const name) : _path{ std::filesystem::path{ testing::TempDir() } / "game-data" / name } {
    std::filesystem::remove_all(_path);
    std::filesystem::create_directories(_path);
  }
  auto Write(std::string_view const name, std::string_view const content) const -> void {
    std::ofstream{ _path / name, std::ios::binary } << content;
  }
  [[nodiscard]] auto Path() const -> std::filesystem::path const& {
    return _path;
  }

private:
  std::filesystem::path _path;
};

[[nodiscard]] auto Verdicts(DirectoryCheck const& check) -> std::vector<FileVerdict> {
  return check.files | std::views::transform(&FileCheck::verdict) | std::ranges::to<std::vector>();
}

}

TEST(GameData, ReadsTheExpectedFilesInAnyCaseAndIgnoresTheRest) {
  auto const directory{ Directory{ "any-case" } };
  directory.Write("one.dat", ONE_CONTENT);
  directory.Write("Two", TWO_CONTENT);
  directory.Write("MAIN.EXE", "not data");
  auto const expected{ TwoFiles() };
  auto const data{ GameData::FromDirectory(directory.Path(), expected) };
  ASSERT_TRUE(data.has_value());
  EXPECT_EQ(data->Names(), (std::vector<std::string>{ "ONE.DAT", "TWO" }));
  EXPECT_EQ(data->File("TWO").value().size(), TWO_CONTENT.size());
  EXPECT_FALSE(data->File("MAIN.EXE").has_value());
}

TEST(GameData, AnswersDosNamesWithoutDriveOrDirectoryInAnyCase) {
  auto const directory{ Directory{ "dos-names" } };
  directory.Write("ONE.DAT", ONE_CONTENT);
  directory.Write("TWO", TWO_CONTENT);
  auto const expected{ TwoFiles() };
  auto const data{ GameData::FromDirectory(directory.Path(), expected).value() };
  EXPECT_EQ(data.DosFile("c:\\opeu\\one.dat").value_or(std::span<std::uint8_t const>{}).size(), ONE_CONTENT.size());
  EXPECT_EQ(data.DosFile("data/Two").value_or(std::span<std::uint8_t const>{}).size(), TWO_CONTENT.size());
  EXPECT_FALSE(data.DosFile("SAVEDATA").has_value());
  EXPECT_FALSE(data.DosFile("").has_value());
}

TEST(GameData, KeepsItsBytesWhenMoved) {
  auto const directory{ Directory{ "moved" } };
  directory.Write("ONE.DAT", ONE_CONTENT);
  directory.Write("TWO", TWO_CONTENT);
  auto const expected{ TwoFiles() };
  auto       data{ GameData::FromDirectory(directory.Path(), expected).value() };
  auto const moved{ std::move(data) };
  EXPECT_TRUE(std::ranges::equal(moved.File("ONE.DAT").value(), BytesOf(ONE_CONTENT)));
}

TEST(GameData, RefusesADirectoryThatLacksAFileOrHoldsAnotherCopy) {
  auto const directory{ Directory{ "faults" } };
  directory.Write("ONE.DAT", "the first filE");
  auto const expected{ TwoFiles() };
  auto const data{ GameData::FromDirectory(directory.Path(), expected) };
  ASSERT_FALSE(data.has_value());
  ASSERT_EQ(data.error().checked.size(), 1U);
  EXPECT_EQ(Verdicts(data.error().checked.front()), (std::vector{ FileVerdict::OTHER_EDITION, FileVerdict::MISSING }));
  directory.Write("ONE.DAT", "short");
  EXPECT_EQ(Verdicts(CheckDirectory(directory.Path(), expected)).front(), FileVerdict::WRONG_SIZE);
}

TEST(GameData, ReportsEveryDirectoryTriedAndTheClosestFileByFile) {
  auto const empty{ Directory{ "empty" } };
  auto const half{ Directory{ "half" } };
  half.Write("ONE.DAT", ONE_CONTENT);
  auto const expected{ TwoFiles() };
  auto const candidates{
    std::array{ empty.Path(), half.Path(), empty.Path() / "absent" }
  };
  auto const found{ FindGameData(candidates, expected) };
  ASSERT_FALSE(found.has_value());
  auto const text{ found.error().Text() };
  EXPECT_EQ(found.error().checked.size(), 3U);
  EXPECT_TRUE(text.contains(RELEASE)) << text;
  EXPECT_TRUE(text.contains("cannot be listed")) << text;
  EXPECT_TRUE(text.contains(half.Path().string() + ":\n  ONE.DAT")) << text;
  EXPECT_TRUE(text.contains("TWO               31 bytes  missing")) << text;
}

TEST(GameData, TakesTheFirstCompleteCandidate) {
  auto const half{ Directory{ "first-half" } };
  half.Write("TWO", TWO_CONTENT);
  auto const whole{ Directory{ "first-whole" } };
  whole.Write("ONE.DAT", ONE_CONTENT);
  whole.Write("TWO", TWO_CONTENT);
  auto const expected{ TwoFiles() };
  auto const candidates{
    std::array{ half.Path(), whole.Path() }
  };
  EXPECT_TRUE(FindGameData(candidates, expected).has_value());
}

TEST(GameData, WrapsATableWithoutCopying) {
  auto const bytes{ std::to_array<std::uint8_t>({ 1, 2, 3 }) };
  auto       table{ GameData::Table{} };
  table.emplace("ONE.DAT", bytes);
  auto const data{ GameData::FromTable(std::move(table)) };
  EXPECT_EQ(data.DosFile("one.dat").value_or(std::span<std::uint8_t const>{}).data(), bytes.data());
  EXPECT_EQ(data.File("ONE.DAT").value().size(), bytes.size());
}

TEST(GameData, TheCompiledTableDescribesTheShippedFiles) {
  auto const data{ GameData::FromDirectory(test::SHIPPED_DIRECTORY, DATA_FILES) };
  ASSERT_TRUE(data.has_value()) << data.error().Text();
  EXPECT_EQ(data->Names().size(), 26U);
}

}
