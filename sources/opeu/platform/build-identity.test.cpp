#include "opeu/platform/build-identity.hpp"

#include <gtest/gtest.h>

#include <regex>
#include <string>

namespace opeu::platform::detail::build_identity_test {

TEST(BuildIdentity, StampsEveryFieldTheDriverWrites) {
  auto const& build{ ThisBuild() };
  EXPECT_FALSE(build.version.empty());
  EXPECT_FALSE(build.commit.empty());
  EXPECT_GE(build.number, 1u);
  EXPECT_TRUE(std::regex_match(std::string{ build.time }, std::regex{ R"([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}Z)" })) << build.time;
  // An exact tag is what describe prints for that commit, dirt aside.
  if (!build.tag.empty()) {
    EXPECT_TRUE(build.version.starts_with(build.tag)) << build.version;
  }
  EXPECT_EQ(build.dirty, build.version.ends_with("-dirty")) << build.version;
}

TEST(BuildIdentity, DescribesVersionNumberAndTime) {
  auto const build{
    BuildIdentity{ .version = "v1.2.3-4-gabcdef0", .commit = "abcdef0", .tag = {}, .time = "2026-10-09T21:05Z", .dirty = false, .number = 17u }
  };
  EXPECT_EQ(Describe(build), "opeu v1.2.3-4-gabcdef0 (build 17, 2026-10-09T21:05Z)");
}

TEST(BuildIdentity, LeavesOutATimeTheStampDidNotCarry) {
  auto const build{
    BuildIdentity{ .version = "unknown", .commit = "unknown", .tag = {}, .time = {}, .dirty = false, .number = 0u }
  };
  EXPECT_EQ(Describe(build), "opeu unknown (build 0)");
}

}
