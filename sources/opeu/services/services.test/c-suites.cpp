// The services' C suites (test_vgalib.c, test_syslib_gfx.c): each returns
// non-zero after printing every failed check.
#include <gtest/gtest.h>

extern "C" auto opeu_test_vgalib() -> int;
extern "C" auto opeu_test_syslib_gfx() -> int;

TEST(Vgalib, DrawsAsTheOriginalLibrary) {
  EXPECT_EQ(opeu_test_vgalib(), 0);
}

TEST(SyslibGfx, ComposesAsTheOriginalLibrary) {
  EXPECT_EQ(opeu_test_syslib_gfx(), 0);
}
