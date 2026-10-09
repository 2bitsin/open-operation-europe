#include "opeu/originals/programs.hpp"

#include "opeu/contract.test.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <ranges>
#include <string>
#include <utility>

namespace opeu::originals::detail::programs_test {

namespace {

namespace fs = std::filesystem;

// Tests run with cwd = this module's directory; the checkout root is the
// nearest ancestor holding buildutil.toml, and binaries/ is beside it.
auto OriginalsDirectory() -> fs::path {
  for (auto directory{ fs::current_path() };;
       directory = directory.parent_path()) {
    if (fs::exists(directory / "buildutil.toml")) {
      return directory / "binaries";
    }
    if (directory == directory.parent_path()) {
      return { };
    }
  }
}

struct MzHeader {
  std::uint16_t last_page_bytes{ };  // e_cblp, 0 = the last page is full
  std::uint16_t pages{ };            // e_cp, 512-byte pages incl. header
  std::uint16_t header_bytes{ };     // e_cparhdr * 16
};

auto ReadMzHeader(fs::path const& path) -> std::optional<MzHeader> {
  auto file{ std::ifstream{ path, std::ios::binary } };
  auto bytes{ std::array<char, 10uz>{ } };
  if (!file.read(bytes.data(), bytes.size()) || bytes[0] != 'M' ||
      bytes[1] != 'Z') {
    return std::nullopt;
  }
  auto const word{ [&](std::size_t at) {
    return static_cast<std::uint16_t>(
        std::bit_cast<unsigned char>(bytes[at]) |
        std::bit_cast<unsigned char>(bytes[at + 1uz]) << 8u);
  } };
  return MzHeader{ word(2uz), word(4uz),
                   static_cast<std::uint16_t>(word(8uz) * 16u) };
}

}

TEST(OriginalPrograms, DescribeTheFilesInTheGameDirectory) {
  auto const directory{ OriginalsDirectory() };
  ASSERT_FALSE(directory.empty()) << "no buildutil.toml above the cwd";
  for (auto const& program : Programs()) {
    auto const path{ directory / program.name };
    ASSERT_TRUE(fs::exists(path)) << path;
    EXPECT_EQ(fs::file_size(path), program.file_bytes) << program.name;
    auto const header{ ReadMzHeader(path) };
    EXPECT_EQ(header.has_value(), program.format == ProgramFormat::MZ)
        << program.name;
    if (header) {
      auto const image_end{ header->last_page_bytes == 0u
                                ? header->pages * 512uz
                                : (header->pages - 1uz) * 512uz +
                                      header->last_page_bytes };
      EXPECT_EQ(image_end, program.file_bytes) << program.name;
      EXPECT_EQ(program.MzHeaderBytes(), header->header_bytes) << program.name;
    } else {
      EXPECT_EQ(program.load_image_bytes, program.file_bytes) << program.name;
    }
  }
}

TEST(OriginalPrograms, FindsADosNameInAnyLetterCase) {
  EXPECT_EQ(ProgramNamed("MAIN.EXE")->load_image_bytes, 341'385uz);
  EXPECT_EQ(ProgramNamed("main.exe"), ProgramNamed("MAIN.EXE"));
  EXPECT_EQ(ProgramNamed("FmDrv.Com")->format, ProgramFormat::COM);
  EXPECT_FALSE(ProgramNamed("OPEU.BAT").has_value());
  EXPECT_FALSE(ProgramNamed("").has_value());
  EXPECT_FALSE(ProgramNamed("MAIN.EXE.BAK").has_value());
}

TEST(OriginalPrograms, LaunchesOnlyProgramsItKnowsAndNotItself) {
  for (auto const name : LAUNCH_ORDER) {
    EXPECT_TRUE(ProgramNamed(name).has_value()) << name;
  }
  EXPECT_FALSE(std::ranges::contains(LAUNCH_ORDER, "KOEI.COM"));
  EXPECT_FALSE(std::ranges::contains(LAUNCH_ORDER, "KOEI.EXE"));
}

TEST(OriginalPrograms, NamesEachFormat) {
  EXPECT_EQ(FormatName(ProgramFormat::MZ), "MZ");
  EXPECT_EQ(FormatName(ProgramFormat::COM), "COM");
}

TEST(OriginalProgramsContract, AComProgramHasNoMzHeader) {
  auto const driver{ *ProgramNamed("FMDRV.COM") };
  EXPECT_CONTRACT_PRECONDITION(static_cast<void>(driver.MzHeaderBytes()),
                               "a COM program has no MZ header");
}

TEST(OriginalProgramsContract, AFormatOutsideTheEnumerationIsUnreachable) {
  EXPECT_CONTRACT_UNREACHABLE(
      static_cast<void>(FormatName(static_cast<ProgramFormat>(7u))), "7");
}

}
