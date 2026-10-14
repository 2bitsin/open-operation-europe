// Battle report formats: the outcome of a battle with its date and commander (the data segment holds the far
// pointers that select them).
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::fbattle {

inline constexpr auto BATTLE_REPORT_0{ std::to_array("On %d/%2d, enemy Commander %s was killed in battle. His army was routed.") };
inline constexpr auto BATTLE_REPORT_1{ std::to_array("On %d/%2d, Commander %s scored a sweeping victory. The enemy\n"
                                                     "fled the area and a formidable line of defense was established.") };
inline constexpr auto BATTLE_REPORT_2{ std::to_array("On %d/%2d, Commander %s pulled off a victory. Though the\n"
                                                     "military gain was small, the victory was considered a forward step.") };
inline constexpr auto BATTLE_REPORT_3{ std::to_array("On %d/%2d, Commander %s won by a narrow margin. Though it\n"
                                                     "was a victory, we needed time to bring the area under control.") };
inline constexpr auto BATTLE_REPORT_4{ std::to_array("On %d/%2d, Commander %s was killed in action.") };
inline constexpr auto BATTLE_REPORT_5{ std::to_array("On %d/%2d, Commander %s's army was annihilated.") };
inline constexpr auto BATTLE_REPORT_6{ std::to_array("On %d/%2d, Commander %s was defeated.") };

} // namespace opeu::game::detail::fbattle

namespace opeu::game {
using detail::fbattle::BATTLE_REPORT_0;
using detail::fbattle::BATTLE_REPORT_1;
using detail::fbattle::BATTLE_REPORT_2;
using detail::fbattle::BATTLE_REPORT_3;
using detail::fbattle::BATTLE_REPORT_4;
using detail::fbattle::BATTLE_REPORT_5;
using detail::fbattle::BATTLE_REPORT_6;
} // namespace opeu::game
