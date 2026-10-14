// Where a pack's files come from: the directory tree, the .opeupack archive, memory.
#include "packs.hpp"

#include "opeu/assets/pack-files.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <iterator>
#include <ranges>
#include <string>
#include <vector>

namespace opeu::assets {

namespace {

using test::ArchiveOf;
using test::CLASSIC_DIRECTORY;
using test::ClassicFile;
using test::SampleArchive;
using test::SamplePaths;
using test::ToBytes;

auto Fault(Packed<ArchivePackFiles> const& archive) -> PackFault {
  return archive ? PackFault{} : archive.error().fault;
}

// An archive of the given directory text and body.
auto Archive(std::string_view const directory, std::string_view const body) -> std::vector<std::byte> {
  return ArchiveOf(directory, ToBytes(body));
}

auto Sorted(std::vector<std::string> paths) -> std::vector<std::string> {
  std::ranges::sort(paths);
  return paths;
}

}

TEST(ArchivePackFiles, HoldTheDirectoryFormsFiles) {
  auto const archive{ ArchivePackFiles::FromBytes(SampleArchive()) };
  ASSERT_TRUE(archive) << Describe(archive.error());
  EXPECT_EQ(archive->Paths(), Sorted(SamplePaths()));
  EXPECT_TRUE(std::ranges::all_of(archive->Paths(), [&](std::string const& path) { return archive->Read(path) == ClassicFile(path); }));
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
  EXPECT_EQ(Fault(ArchivePackFiles::Open("assets.test/no-such.opeupack")), PackFault::MISSING_FILE);
}

TEST(DirectoryPackFiles, ReadFilesUnderTheRoot) {
  auto const files{ DirectoryPackFiles{ CLASSIC_DIRECTORY } };
  EXPECT_TRUE(files.Read("pack.json"));
  EXPECT_EQ(files.Read("no-such.png").error().fault, PackFault::MISSING_FILE);
}

TEST(DirectoryPackFiles, RefusePathsThatLeaveThePack) {
  auto const files{ DirectoryPackFiles{ CLASSIC_DIRECTORY } };
  EXPECT_EQ(files.Read("../../CMakeLists.txt").error().fault, PackFault::BAD_MANIFEST);
  EXPECT_EQ(files.Read("/etc/hosts").error().fault, PackFault::BAD_MANIFEST);
}

TEST(MemoryPackFiles, ReadWhatTheyHold) {
  auto const files{ MemoryPackFiles{ { { "a/b.png", ToBytes("png") } } } };
  EXPECT_EQ(files.Read("a/b.png"), ToBytes("png"));
  EXPECT_EQ(files.Read("a/c.png").error().fault, PackFault::MISSING_FILE);
}

}
