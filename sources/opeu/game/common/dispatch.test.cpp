// The dispatch tables: a key runs its row's handler with the arguments and returns its result; a key no row holds
// breaks the contract instead of running anything.
#include "common/dispatch.hpp"

#include "opeu/contract.test.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace opeu::game::detail::dispatch_test {

enum class Slot : std::uint32_t {
  NONE   = 0x00000000,
  DOUBLE = 0x0d920008,
  NEGATE = 0x0d920010,
  ABSENT = 0x0d920018,
};

auto Double(int value) -> int {
  return value * 2;
}

auto Negate(int value) -> int {
  return -value;
}

constexpr auto TABLE = DispatchRows<Slot, auto (*)(int)->int>({
    { Slot::DOUBLE, Double },
    { Slot::NEGATE, Negate },
});

TEST(Dispatch, RunsTheHandlerOfTheKeysRow) {
  EXPECT_EQ(Dispatch(TABLE, Slot::DOUBLE, 21), 42);
  EXPECT_EQ(Dispatch(TABLE, Slot::NEGATE, 7), -7);
}

TEST(Dispatch, CountsTheRowsItIsWrittenWith) {
  static_assert(TABLE.size() == 2);
  EXPECT_EQ(HandlerOf(TABLE, Slot::NEGATE), &Negate);
}

TEST(Dispatch, AKeyNoRowHoldsIsAContractFailure) {
  opeu::ExpectContractUnreachable([] { static_cast<void>(Dispatch(TABLE, Slot::ABSENT, 1)); }, "227672088");
}
} // namespace opeu::game::detail::dispatch_test
