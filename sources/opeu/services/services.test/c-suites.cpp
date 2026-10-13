// The services' C suites (test-vgalib.cpp, test-syslib-gfx.cpp): each returns
// non-zero after printing every failed check.
#include <gtest/gtest.h>

auto TestVgalib() -> int;
auto TestSyslibGfx() -> int;

TEST(Vgalib, DrawsAsTheOriginalLibrary) {
  EXPECT_EQ(TestVgalib(), 0);
}

TEST(SyslibGfx, ComposesAsTheOriginalLibrary) {
  EXPECT_EQ(TestSyslibGfx(), 0);
}
