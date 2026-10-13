// The save directory's files: DOS names found case-insensitively, created in upper case, read, written and sought.
#include "opeu/platform/platform.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>

namespace opeu::platform::detail::save_file_test {

class SaveFiles : public ::testing::Test {
protected:
  auto SetUp() -> void override {
    _saves = std::filesystem::temp_directory_path() / ("opeu-save-file-" + std::string{ ::testing::UnitTest::GetInstance()->current_test_info()->name() });
    std::filesystem::remove_all(_saves);
    plat_set_save_dir(_saves.string());
  }
  auto TearDown() -> void override {
    std::filesystem::remove_all(_saves);
  }
  std::filesystem::path _saves;
};

TEST_F(SaveFiles, CreateTheDosSpellingAndReadItBack) {
  auto created{ SaveFile::Open("Savedata", FileMode::CREATE) };
  ASSERT_TRUE(created);
  std::array<std::uint8_t, 3> const bytes{ 1, 2, 3 };
  EXPECT_EQ(created->Write(bytes), 3U);
  created.reset();
  EXPECT_TRUE(std::filesystem::exists(_saves / "SAVEDATA"));
  auto read{ SaveFile::Open("savedata", FileMode::READ) };
  ASSERT_TRUE(read);
  std::array<std::uint8_t, 8> buffer{};
  EXPECT_EQ(read->Read(buffer), 3U);
  EXPECT_EQ(buffer[2], 3);
}

TEST_F(SaveFiles, OpenOnlyWhatExistsUnlessCreating) {
  EXPECT_FALSE(SaveFile::Open("MISSING", FileMode::READ));
  EXPECT_FALSE(SaveFile::Open("MISSING", FileMode::READ_WRITE));
  EXPECT_FALSE(SaveFile::Open("NO\\DIR", FileMode::CREATE));
}

TEST_F(SaveFiles, SeekFromEveryOriginButNotBeforeTheStart) {
  {
    std::ofstream{ _saves / "DATA", std::ios::binary } << "abcdef";
  }
  auto file{ SaveFile::Open("data", FileMode::READ_WRITE) };
  ASSERT_TRUE(file);
  EXPECT_EQ(file->Seek(2, SeekOrigin::START), 2);
  EXPECT_EQ(file->Seek(1, SeekOrigin::CURRENT), 3);
  EXPECT_EQ(file->Seek(-1, SeekOrigin::END), 5);
  EXPECT_FALSE(file->Seek(-9, SeekOrigin::START));
  std::array<std::uint8_t, 1> const x{ 'X' };
  EXPECT_EQ(file->Write(x), 1U);
  file->Flush();
  std::string contents;
  std::ifstream{ _saves / "DATA", std::ios::binary } >> contents;
  EXPECT_EQ(contents, "abcdeX");
}

}
