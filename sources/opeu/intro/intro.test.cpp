// The entry's option pass for the opening: the last of --intro and --no-intro wins, the other arguments stay in order.
#include "opeu/intro/intro.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string_view>
#include <vector>

namespace opeu::intro::detail::intro_test {
namespace {

using namespace std::string_view_literals;

TEST(IntroOptions, AskNothingWithoutAFlag) {
  constexpr auto ARGUMENTS{
    std::array{ "opeu"sv, "--headless"sv, "--ms"sv, "100"sv }
  };
  auto const options{ IntroOptions(ARGUMENTS) };
  EXPECT_EQ(options.request, IntroRequest::IN_A_WINDOW_ONLY);
  EXPECT_EQ(options.remaining, (std::vector<std::string_view>{ ARGUMENTS.begin(), ARGUMENTS.end() }));
}

TEST(IntroOptions, TakeTheLastFlagOutOfTheArguments) {
  constexpr auto ARGUMENTS{
    std::array{ "opeu"sv, "--no-intro"sv, "--headless"sv, "--intro"sv, "--mute"sv }
  };
  auto const options{ IntroOptions(ARGUMENTS) };
  EXPECT_EQ(options.request, IntroRequest::RUN);
  EXPECT_EQ(options.remaining, (std::vector{ "opeu"sv, "--headless"sv, "--mute"sv }));
}

TEST(IntroOptions, SkipOnNoIntro) {
  constexpr auto ARGUMENTS{
    std::array{ "opeu"sv, "--intro"sv, "--no-intro"sv }
  };
  EXPECT_EQ(IntroOptions(ARGUMENTS).request, IntroRequest::SKIP);
}

TEST(IntroOptions, KeepTheProgramNameEvenWhenItLooksLikeAFlag) {
  constexpr auto ARGUMENTS{ std::array{ "--intro"sv } };
  auto const     options{ IntroOptions(ARGUMENTS) };
  EXPECT_EQ(options.request, IntroRequest::IN_A_WINDOW_ONLY);
  EXPECT_EQ(options.remaining, (std::vector{ "--intro"sv }));
}

}
}
