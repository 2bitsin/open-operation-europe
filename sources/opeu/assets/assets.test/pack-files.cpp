// Where a pack's files come from: the directory tree, the .opeupack archive, memory.
#include "packs.hpp"

#include "opeu/assets/pack-files.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::SAMPLE_ARCHIVE;
using test::SAMPLE_DIRECTORY;
using test::SampleFile;
using test::ToBytes;

auto Fault(Packed<ArchivePackFiles> const& archive) -> PackFault {
  return archive ? PackFault{} : archive.error().fault;
}

// An archive of the given directory text and body.
auto Archive(std::string_view const directory, std::string_view const body) -> std::vector<std::byte> {
  auto       bytes{ ToBytes("OPEUPACK") };
  auto const length{ directory.size() };
  std::ranges::transform(std::array{ 0, 8, 16, 24 }, std::back_inserter(bytes), [length](int const shift) { return static_cast<std::byte>(length >> shift & 0xFF); });
  std::ranges::copy(ToBytes(directory), std::back_inserter(bytes));
  std::ranges::copy(ToBytes(body), std::back_inserter(bytes));
  return bytes;
}

auto SampleDirectoryPaths() -> std::vector<std::string> {
  auto paths{ std::filesystem::recursive_directory_iterator{ SAMPLE_DIRECTORY } | std::views::filter([](auto const& entry) { return entry.is_regular_file(); })
              | std::views::transform([](auto const& entry) { return entry.path().lexically_relative(SAMPLE_DIRECTORY).generic_string(); }) | std::ranges::to<std::vector>() };
  std::ranges::sort(paths);
  return paths;
}

}

TEST(ArchivePackFiles, HoldTheDirectoryFormsFiles) {
  auto const archive{ ArchivePackFiles::Open(SAMPLE_ARCHIVE) };
  ASSERT_TRUE(archive) << Describe(archive.error());
  EXPECT_EQ(archive->Paths(), SampleDirectoryPaths());
  EXPECT_TRUE(std::ranges::all_of(archive->Paths(), [&](std::string const& path) { return archive->Read(path) == SampleFile(path); }));
}

TEST(ArchivePackFiles, ReadAWellFormedArchive) {
  auto const archive{ ArchivePackFiles::FromBytes(Archive(R"({"format":"opeu-archive/1","files":[{"path":"a","at":0,"bytes":2},{"path":"b","at":2,"bytes":1}]})", "xyz")) };
  ASSERT_TRUE(archive);
  EXPECT_EQ(archive->Read("a"), ToBytes("xy"));
  EXPECT_EQ(archive->Read("b"), ToBytes("z"));
  EXPECT_EQ(archive->Read("c").error().fault, PackFault::MISSING_FILE);
}

TEST(ArchivePackFiles, RefuseBytesWithoutTheMagic) {
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes(ToBytes("NOTAPACKxxxx"))), PackFault::BAD_ARCHIVE);
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes({})), PackFault::BAD_ARCHIVE);
}

TEST(ArchivePackFiles, RefuseADirectoryPastTheEnd) {
  auto bytes{ Archive("{}", "") };
  bytes[8] = std::byte{ 0x40 };
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes(bytes)), PackFault::BAD_ARCHIVE);
}

TEST(ArchivePackFiles, RefuseAFilePastTheEnd) {
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes(Archive(R"({"format":"opeu-archive/1","files":[{"path":"a","at":1,"bytes":3}]})", "xyz"))), PackFault::BAD_ARCHIVE);
}

TEST(ArchivePackFiles, RefuseADirectoryOfAnotherFormat) {
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes(Archive(R"({"format":"zip","files":[]})", ""))), PackFault::BAD_ARCHIVE);
  EXPECT_EQ(Fault(ArchivePackFiles::FromBytes(Archive("not json", ""))), PackFault::BAD_ARCHIVE);
}

TEST(ArchivePackFiles, SayAMissingArchiveIsMissing) {
  EXPECT_EQ(Fault(ArchivePackFiles::Open("assets.test/packs/no-such.opeupack")), PackFault::MISSING_FILE);
}

TEST(DirectoryPackFiles, ReadFilesUnderTheRoot) {
  auto const files{ DirectoryPackFiles{ SAMPLE_DIRECTORY } };
  EXPECT_TRUE(files.Read("pack.json"));
  EXPECT_EQ(files.Read("no-such.png").error().fault, PackFault::MISSING_FILE);
}

TEST(DirectoryPackFiles, RefusePathsThatLeaveThePack) {
  auto const files{ DirectoryPackFiles{ SAMPLE_DIRECTORY } };
  EXPECT_EQ(files.Read("../classic-sample.opeupack").error().fault, PackFault::BAD_MANIFEST);
  EXPECT_EQ(files.Read("/etc/hosts").error().fault, PackFault::BAD_MANIFEST);
}

TEST(MemoryPackFiles, ReadWhatTheyHold) {
  auto const files{ MemoryPackFiles{ { { "a/b.png", ToBytes("png") } } } };
  EXPECT_EQ(files.Read("a/b.png"), ToBytes("png"));
  EXPECT_EQ(files.Read("a/c.png").error().fault, PackFault::MISSING_FILE);
}

}
