// Staff advice messages: the advisor's remarks and the general tips.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::fadvice {

inline constexpr auto ADVICE_00{ std::to_array("Our priority is the defense of %s.\033S") };
inline constexpr auto ADVICE_01{ std::to_array("This is our chance to take %s.\033S") };
inline constexpr auto ADVICE_02{ std::to_array("We need to decide who will take over the "
                                               "Division %d Commander position.\033S") };
inline constexpr auto ADVICE_03{ std::to_array("You look tired...Why not leave the Division Commander in charge?\033S") };
inline constexpr auto ADVICE_04{ std::to_array("It's too dangerous here. We should move Army HQ.\033S") };
inline constexpr auto ADVICE_05{ std::to_array("Let's stop everything and reorganize.\033S") };
inline constexpr auto ADVICE_06{ std::to_array("We should send help to %s.\033S") };
inline constexpr auto ADVICE_07{ std::to_array("Let's send %s to the frontlines.\033S") };
inline constexpr auto ADVICE_08{ std::to_array("Let's pull back behind %s.\033S") };
inline constexpr auto ADVICE_09{ std::to_array("We're down to our last supplies. We should request more.\033S") };
inline constexpr auto ADVICE_10{ std::to_array("We need to send reinforcements to "
                                               "Division %d, before they're crushed.\033S") };
inline constexpr auto ADVICE_11{ std::to_array("We should plant land mines in order to stop the enemy's advances.\033S") };
inline constexpr auto ADVICE_12{ std::to_array("Let's ask the Air Force for help.\033S") };
inline constexpr auto ADVICE_13{ std::to_array("We should call in the Special Forces Commander.\033S") };
inline constexpr auto ADVICE_14{ std::to_array("I suggest a paratrooper attack at the enemy's rear flank.\033S") };
inline constexpr auto ADVICE_15{ std::to_array("I think we should solidify our defenses at %s.\033S") };
inline constexpr auto ADVICE_16{ std::to_array("We should just rush right in and take\n"
                                               "%s.\033S") };
inline constexpr auto ADVICE_17{ std::to_array("%s will be ready for battle shortly.\033S") };
inline constexpr auto ADVICE_18{ std::to_array("%s's operations will be finished shortly.\033S") };
inline constexpr auto ADVICE_19{ std::to_array("%s is short of %s.\033S") };
inline constexpr auto ADVICE_20{ std::to_array("Division %d Commander, %s, is seriously wounded.\033S") };
inline constexpr auto ADVICE_21{ std::to_array("%s, Division %d Commander, and his men are fatigued.\033S") };
inline constexpr auto ADVICE_22{ std::to_array("%s is being exhausted rather quickly.\033S") };
inline constexpr auto ADVICE_23{ std::to_array("%s has quite a lot of damaged arms.\033S") };
inline constexpr auto ADVICE_24{ std::to_array("Communications with %s are still out.\033S") };
inline constexpr auto ADVICE_25{ std::to_array("%s is exhausted.\033S") };
inline constexpr auto ADVICE_26{ std::to_array("It looks like %s has rested enough.\033S") };
inline constexpr auto ADVICE_27{ std::to_array("The morale in %s is very high.\033S") };
inline constexpr auto ADVICE_28{ std::to_array("The morale in %s is dropping dramatically.\033S") };
inline constexpr auto ADVICE_29{ std::to_array("There is nothing right now.\033S") };
inline constexpr auto ADVICE_30{ std::to_array("It looks like the enemy is making a mine field.\033S") };
inline constexpr auto ADVICE_31{ std::to_array("The enemy fleets are preparing to bombard us.\033S") };
inline constexpr auto ADVICE_32{ std::to_array("The Commander of enemy Division %d, is not fit for battle.\033S") };
inline constexpr auto ADVICE_33{ std::to_array("Be careful of enemy air raids.\033S") };
inline constexpr auto ADVICE_34{ std::to_array("The enemy's paratroop operation has started.\033S") };
inline constexpr auto ADVICE_35{ std::to_array("It looks like the enemy cargo plane have started to load.\033S") };
inline constexpr auto ADVICE_36{ std::to_array("It doesn't look like the enemy has any plans to attack just now.\033S") };
inline constexpr auto ADVICE_37{ std::to_array("The enemy %s attack will come shortly.\033S") };
inline constexpr auto ADVICE_38{ std::to_array("The enemy %s is resting.\033S") };
inline constexpr auto ADVICE_39{ std::to_array("The enemy is plotting to take %s.\033S") };
inline constexpr auto ADVICE_40{ std::to_array("The enemy is strengthening its defenses in %s.\033S") };
inline constexpr auto ADVICE_41{ std::to_array("Enemy recon teams are really starting to get active.\033S") };
inline constexpr auto ADVICE_42{ std::to_array("It would seem that the enemy knows of our plans.\033S") };
inline constexpr auto ADVICE_43{ std::to_array("There are plenty of materials in %s.\033S") };
inline constexpr auto ADVICE_44{ std::to_array("Enemy %s movement seems to be slowing down.\033S") };
inline constexpr auto ADVICE_45{ std::to_array("Enemy %s has been really active recently.\033S") };
inline constexpr auto ADVICE_46{ std::to_array("It seems that Commander %s, is in Division %d.\033S") };
inline constexpr auto ADVICE_47{ std::to_array("We don't have enough information to go on. Send in some recon teams.\033S") };
inline constexpr auto ADVICE_48{ std::to_array("%s is well isolated from the enemy's territories.\033S") };
inline constexpr auto ADVICE_49{ std::to_array("We have no choice but to abandon %s.\033S") };
inline constexpr auto ADVICE_50{ std::to_array("Our forces are isolated within the enemy's territories.\033S") };
inline constexpr auto ADVICE_51{ std::to_array("Our forces are taking some serious casualties.\033S") };
inline constexpr auto ADVICE_52{ std::to_array("We will wait now for our recovery.\033S") };
inline constexpr auto ADVICE_53{ std::to_array("It's just the right time to go on the offensive.\033S") };
inline constexpr auto ADVICE_54{ std::to_array("We will just have to wait it out.\033S") };
inline constexpr auto ADVICE_55{ std::to_array("We should wait for our reinforcements in Division %d to arrive.\033S") };
inline constexpr auto ADVICE_56{ std::to_array("The situation couldn't be worse.\033S") };
inline constexpr auto ADVICE_57{ std::to_array("We are overwhelming the enemy.\033S") };
inline constexpr auto ADVICE_58{ std::to_array("The enemy is very powerful.\033S") };
inline constexpr auto ADVICE_59{ std::to_array("The enemy is just as strong as we are.\033S") };
inline constexpr auto ADVICE_60{ std::to_array("The enemy rules the skies.\033S") };
inline constexpr auto ADVICE_61{ std::to_array("We rule the skies.\033S") };
inline constexpr auto ADVICE_62{ std::to_array("There have been no advances.\033S") };
inline constexpr auto ADVICE_63{ std::to_array("Without control of the skies, we shouldn't hope for too much air "
                                               "support.\033S") };
inline constexpr auto ADVICE_64{ std::to_array("Carpet bombing is usually used to weaken cities.\033S") };
inline constexpr auto ADVICE_65{ std::to_array("Gunfire from the air is especially effective against ground troops.\033S") };
inline constexpr auto ADVICE_66{ std::to_array("Division %d will arrive later with reinforcements.\033S") };
inline constexpr auto ADVICE_67{ std::to_array("Until a new Commander for Division %d is "
                                               "appointed, a Deputy will take over.\033S") };
inline constexpr auto ADVICE_68{ std::to_array("The Chief of Intelligence and Air Commander do not agree very often.\033S") };
inline constexpr auto ADVICE_69{ std::to_array("The winters in Ardennes are horrible.\033S") };
inline constexpr auto ADVICE_70{ std::to_array("It's hurricane season in Normandy now.\033S") };
inline constexpr auto ADVICE_71{ std::to_array("Let's build a bridge and start moving our divisions.\033S") };
inline constexpr auto ADVICE_72{ std::to_array("If we knock out the bridges, it will slow the enemy's advance and buy us "
                                               "some time.\033S") };
inline constexpr auto ADVICE_73{ std::to_array("If you take charge on the front lines, troop morale will increase.\033S") };
inline constexpr auto ADVICE_74{ std::to_array("You should send engineering units to any areas you suspect the enemy has "
                                               "placed mines.\033S") };
inline constexpr auto ADVICE_75{ std::to_array("Units with little food suffer a drop in\n"
                                               "morale.\033S") };
inline constexpr auto ADVICE_76{ std::to_array("It is not wise to send units into battle before they have had a chance to "
                                               "prepare.\033S") };
inline constexpr auto ADVICE_77{ std::to_array("You should supply materials as quickly as possible.\033S") };
inline constexpr auto ADVICE_78{ std::to_array("Special Forces operations are usually more successful at night.\033S") };
inline constexpr auto ADVICE_79{ std::to_array("If we continue to win, Army HQ will "
                                               "certainly look upon us favorably.\033S") };
inline constexpr auto ADVICE_80{ std::to_array("Knowing when to retreat is very important.\033S") };
inline constexpr auto ADVICE_81{ std::to_array("Any success with Air Force negotiations depends on our Chief of "
                                               "Intelligence's charm.\033S") };
inline constexpr auto ADVICE_82{ std::to_array("A successful paratroop operation depends "
                                               "on our Chief of Intelligence.\033S") };
inline constexpr auto ADVICE_83{ std::to_array("We need to concentrate our forces if we want to win.\033S") };
inline constexpr auto ADVICE_84{ std::to_array("We should attack the enemy relentlessly, "
                                               "so they have no time for defense.\033S") };
inline constexpr auto ADVICE_85{ std::to_array("It is very dangerous to let units on the front line rest.\033S") };
inline constexpr auto ADVICE_86{ std::to_array("If a city already has a well-prepared defense, it will be difficult to "
                                               "take quickly.\033S") };
inline constexpr auto ADVICE_87{ std::to_array("Tanks are not designed for long-distance journeys.\033S") };
inline constexpr auto ADVICE_88{ std::to_array("Our forces are only half as powerful at night as they are during the\n"
                                               "day.\033S") };
inline constexpr auto ADVICE_89{ std::to_array("Infantry troops can only carry two days worth of food with them.\033S") };
inline constexpr auto ADVICE_90{ std::to_array("The terrain can either work for you, or against you in a battle.\033S") };
inline constexpr auto ADVICE_91{ std::to_array("We should let the soldiers rest as much as possible at night.\033S") };
inline constexpr auto ADVICE_92{ std::to_array("Attacking at night is a good way to deprive enemy troops any rest.\033S") };

} // namespace opeu::game::detail::fadvice

namespace opeu::game {
using detail::fadvice::ADVICE_00;
using detail::fadvice::ADVICE_01;
using detail::fadvice::ADVICE_02;
using detail::fadvice::ADVICE_03;
using detail::fadvice::ADVICE_04;
using detail::fadvice::ADVICE_05;
using detail::fadvice::ADVICE_06;
using detail::fadvice::ADVICE_07;
using detail::fadvice::ADVICE_08;
using detail::fadvice::ADVICE_09;
using detail::fadvice::ADVICE_10;
using detail::fadvice::ADVICE_11;
using detail::fadvice::ADVICE_12;
using detail::fadvice::ADVICE_13;
using detail::fadvice::ADVICE_14;
using detail::fadvice::ADVICE_15;
using detail::fadvice::ADVICE_16;
using detail::fadvice::ADVICE_17;
using detail::fadvice::ADVICE_18;
using detail::fadvice::ADVICE_19;
using detail::fadvice::ADVICE_20;
using detail::fadvice::ADVICE_21;
using detail::fadvice::ADVICE_22;
using detail::fadvice::ADVICE_23;
using detail::fadvice::ADVICE_24;
using detail::fadvice::ADVICE_25;
using detail::fadvice::ADVICE_26;
using detail::fadvice::ADVICE_27;
using detail::fadvice::ADVICE_28;
using detail::fadvice::ADVICE_29;
using detail::fadvice::ADVICE_30;
using detail::fadvice::ADVICE_31;
using detail::fadvice::ADVICE_32;
using detail::fadvice::ADVICE_33;
using detail::fadvice::ADVICE_34;
using detail::fadvice::ADVICE_35;
using detail::fadvice::ADVICE_36;
using detail::fadvice::ADVICE_37;
using detail::fadvice::ADVICE_38;
using detail::fadvice::ADVICE_39;
using detail::fadvice::ADVICE_40;
using detail::fadvice::ADVICE_41;
using detail::fadvice::ADVICE_42;
using detail::fadvice::ADVICE_43;
using detail::fadvice::ADVICE_44;
using detail::fadvice::ADVICE_45;
using detail::fadvice::ADVICE_46;
using detail::fadvice::ADVICE_47;
using detail::fadvice::ADVICE_48;
using detail::fadvice::ADVICE_49;
using detail::fadvice::ADVICE_50;
using detail::fadvice::ADVICE_51;
using detail::fadvice::ADVICE_52;
using detail::fadvice::ADVICE_53;
using detail::fadvice::ADVICE_54;
using detail::fadvice::ADVICE_55;
using detail::fadvice::ADVICE_56;
using detail::fadvice::ADVICE_57;
using detail::fadvice::ADVICE_58;
using detail::fadvice::ADVICE_59;
using detail::fadvice::ADVICE_60;
using detail::fadvice::ADVICE_61;
using detail::fadvice::ADVICE_62;
using detail::fadvice::ADVICE_63;
using detail::fadvice::ADVICE_64;
using detail::fadvice::ADVICE_65;
using detail::fadvice::ADVICE_66;
using detail::fadvice::ADVICE_67;
using detail::fadvice::ADVICE_68;
using detail::fadvice::ADVICE_69;
using detail::fadvice::ADVICE_70;
using detail::fadvice::ADVICE_71;
using detail::fadvice::ADVICE_72;
using detail::fadvice::ADVICE_73;
using detail::fadvice::ADVICE_74;
using detail::fadvice::ADVICE_75;
using detail::fadvice::ADVICE_76;
using detail::fadvice::ADVICE_77;
using detail::fadvice::ADVICE_78;
using detail::fadvice::ADVICE_79;
using detail::fadvice::ADVICE_80;
using detail::fadvice::ADVICE_81;
using detail::fadvice::ADVICE_82;
using detail::fadvice::ADVICE_83;
using detail::fadvice::ADVICE_84;
using detail::fadvice::ADVICE_85;
using detail::fadvice::ADVICE_86;
using detail::fadvice::ADVICE_87;
using detail::fadvice::ADVICE_88;
using detail::fadvice::ADVICE_89;
using detail::fadvice::ADVICE_90;
using detail::fadvice::ADVICE_91;
using detail::fadvice::ADVICE_92;
} // namespace opeu::game
