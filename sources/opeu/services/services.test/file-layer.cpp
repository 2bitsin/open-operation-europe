// fileio.c over the embedded table: shipped files from memory, the save
// directory first, writes only there (a shipped file copied on its first write).
#include "opeu/services/asset-table.h"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

extern "C" {
auto opeu_dos_open(char const* name, std::uint16_t mode) -> std::int16_t;
auto opeu_dos_close(std::int16_t handle) -> std::int16_t;
auto opeu_dos_read(std::int16_t handle, std::uint8_t* data, std::uint16_t size) -> std::int16_t;
auto opeu_dos_write(std::int16_t handle, std::uint8_t const* data, std::uint16_t size) -> std::int16_t;
auto opeu_dos_seek(std::int16_t handle, std::int32_t position, std::int16_t origin) -> std::int32_t;
auto opeu_dos_create(char const* name) -> std::int16_t;
auto opeu_dos_exists(char const* name) -> std::int16_t;
void plat_set_save_dir(char const* dir);
}

namespace {

constexpr auto SHIPPED{ std::array<unsigned char, 6>{ 'S', 'H', 'I', 'P', 'P', 'D' } };

auto Find(char const* const name, unsigned char const** const data, std::size_t* const size) -> int {
  if (std::strcmp(name, "SCEDATA") != 0) {
    return 0;
  }
  *data = SHIPPED.data();
  *size = SHIPPED.size();
  return 1;
}

auto Contents(std::filesystem::path const& path) -> std::string {
  auto in{ std::ifstream{ path, std::ios::binary } };
  return { std::istreambuf_iterator<char>{ in }, {} };
}

class FileLayer : public ::testing::Test {
protected:
  void SetUp() override {
    saves_ = std::filesystem::temp_directory_path()
           / ("opeu-file-layer-" + std::string{ ::testing::UnitTest::GetInstance()->current_test_info()->name() });
    std::filesystem::remove_all(saves_);
    plat_set_save_dir(saves_.string().c_str());
    opeu_files_use_assets(&Find);
  }
  void TearDown() override { std::filesystem::remove_all(saves_); }
  std::filesystem::path saves_;
};

TEST_F(FileLayer, ReadsAShippedFileFromMemory) {
  auto const h{ opeu_dos_open("SCEDATA", 2) };
  ASSERT_GE(h, 5);
  std::array<std::uint8_t, 8> buffer{};
  EXPECT_EQ(opeu_dos_seek(h, 2, 0), 2);
  EXPECT_EQ(opeu_dos_read(h, buffer.data(), 8), 4);
  EXPECT_EQ(std::string(buffer.begin(), buffer.begin() + 4), "IPPD");
  EXPECT_EQ(opeu_dos_seek(h, -1, 2), 5);
  EXPECT_EQ(opeu_dos_close(h), 0);
  EXPECT_FALSE(std::filesystem::exists(saves_ / "SCEDATA"));
  EXPECT_EQ(opeu_dos_exists("SCEDATA"), 1);
  EXPECT_EQ(opeu_dos_open("MISSING", 0), -1);
}

TEST_F(FileLayer, CopiesAShippedFileOnItsFirstWrite) {
  auto const h{ opeu_dos_open("SCEDATA", 2) };
  ASSERT_GE(h, 5);
  EXPECT_EQ(opeu_dos_seek(h, 4, 0), 4);
  std::array<std::uint8_t, 2> const two{ 'E', 'D' };
  EXPECT_EQ(opeu_dos_write(h, two.data(), 2), 2);
  opeu_dos_close(h);
  EXPECT_EQ(Contents(saves_ / "SCEDATA"), "SHIPED");
  auto const again{ opeu_dos_open("SCEDATA", 0) };  // the save directory shadows the table
  std::array<std::uint8_t, 6> buffer{};
  EXPECT_EQ(opeu_dos_read(again, buffer.data(), 6), 6);
  EXPECT_EQ(std::string(buffer.begin(), buffer.end()), "SHIPED");
  opeu_dos_close(again);
}

TEST_F(FileLayer, CreatesSavedGamesInTheSaveDirectory) {
  auto const h{ opeu_dos_create("Savedata") };
  ASSERT_GE(h, 5);
  std::array<std::uint8_t, 3> const bytes{ 1, 2, 3 };
  EXPECT_EQ(opeu_dos_write(h, bytes.data(), 3), 3);
  opeu_dos_close(h);
  EXPECT_EQ(Contents(saves_ / "SAVEDATA").size(), 3U);
  EXPECT_EQ(opeu_dos_exists("SAVEDATA"), 1);
}

}  // namespace
