// The services' C suites (test-vgalib.cpp, test-syslib-gfx.cpp): each returns
// non-zero after printing every failed check.
#include <gtest/gtest.h>

auto opeu_test_vgalib() -> int;
auto opeu_test_syslib_gfx() -> int;

TEST(Vgalib, DrawsAsTheOriginalLibrary) {
  EXPECT_EQ(opeu_test_vgalib(), 0);
}

TEST(SyslibGfx, ComposesAsTheOriginalLibrary) {
  EXPECT_EQ(opeu_test_syslib_gfx(), 0);
}
