#include "opeu/game/core/evwound.hpp"
#include "opeu/game/common/data-segment.hpp"
#include "opeu/game/common/game-state.hpp"
#include "opeu/game/common/game.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>

namespace opeu::game::detail::evwound_test {
namespace {

constexpr std::uint8_t SIDE_0_NATION{ 0x01 };

// The initial state (the notice's prefix and suffix are its texts), its first general a side-0 general named `name`
// on a side a player commands.
class Fixture {
public:
  explicit Fixture(std::string_view name) {
    auto& data{ Data(_game) };
    auto& general{ data.people.front() };
    std::ranges::fill(general.name, '\0');
    std::ranges::copy(name, general.name.begin());
    data.side_masks[0]  = SIDE_0_NATION;
    general.status      = SIDE_0_NATION;
    data.side_status[0] = SIDE_PLAYED;
  }

  [[nodiscard]] auto Notice() -> DeathNotice {
    return ComposeDeathNotice(_game, Near(Data(_game), Data(_game).people.front()));
  }

private:
  GameState         _state{ GameState::Initial() };
  services::Machine _machine;
  game::Game        _game{ _state, _machine };
};

constexpr std::string_view SUFFIX{ "\012has died from his\012wounds, sir.\033S" };

TEST(DeathNotice, NamesTheGeneralAndKeepsHisSideForAShortName) {
  Fixture fixture{ "Model" };
  auto    notice{ fixture.Notice() };
  EXPECT_EQ(notice.Message().View(), std::string{ "Commander Model" } + std::string{ SUFFIX });
  EXPECT_EQ(notice.Side(), 0);
}

TEST(DeathNotice, RunsALongNameIntoTheSideByteAndPastTheOriginalFrame) {
  Fixture           fixture{ "von Rundstedt-Mai" }; // 17 characters, the longest a name record holds
  auto              notice{ fixture.Notice() };
  std::string const expected{ std::string{ "Commander von Rundstedt-Mai" } + std::string{ SUFFIX } };
  EXPECT_EQ(notice.Message().View(), expected);
  EXPECT_GT(expected.size(), DeathNotice::SIDE_AT);
  EXPECT_EQ(notice.Side(), static_cast<std::uint8_t>(expected[DeathNotice::SIDE_AT]));
}
} // namespace
} // namespace opeu::game::detail::evwound_test
