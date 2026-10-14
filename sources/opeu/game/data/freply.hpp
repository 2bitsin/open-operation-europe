// Mission results, officer replies and status-line formats, selected through the data segment's far pointer tables.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::freply {

inline constexpr auto REPLY_000{ std::to_array("\033C%c%s Special Forces \033C7successfully completed %s.\033S") };
inline constexpr auto REPLY_001{ std::to_array("\033C%c%s SpFc team attack:\n"
                                               "\033C7%s\033S") };
inline constexpr auto REPLY_002{ std::to_array("\033C%c%s Special Forces \033C7failed %s.\033S") };
inline constexpr auto REPLY_003{ std::to_array(" has been assassinated!\033S") };
inline constexpr auto REPLY_004{ std::to_array("Well, you and your men have your orders, so I'm afraid\n") };
inline constexpr auto REPLY_005{ std::to_array("Preparations will take\n"
                                               "about %d hours, but\n"
                                               "we'll try to speed things up.\033S") };
inline constexpr auto REPLY_006{ std::to_array(" assassination mission") };
inline constexpr auto REPLY_007{ std::to_array("this is goodbye for now. Hopefully this will all be\n") };
inline constexpr auto REPLY_008{ std::to_array("Let me offer my deepest congratulations, Commander!\n") };
inline constexpr auto REPLY_009{ std::to_array("You and your men have done a marvelous job. You\n") };
inline constexpr auto REPLY_010{ std::to_array("should all be very proud of yourselves. Your country\n") };
inline constexpr auto REPLY_011{ std::to_array("thanks you for your honorable service.") };
inline constexpr auto REPLY_012{ std::to_array("At ease men. I'm not here on any serious business.\033S") };
inline constexpr auto REPLY_013{ std::to_array("I wanted to see you and your men in "
                                               "action. So I came to the front line.\033S") };
inline constexpr auto REPLY_014{ std::to_array("And by gone it! Your men are putting up one hell of a fight!") };
inline constexpr auto REPLY_015{ std::to_array("With you in charge here, it won't be long "
                                               "before they run crying home.\033S") };
inline constexpr auto REPLY_016{ std::to_array("I'm counting on you. Keep up the good work.\033S") };
inline constexpr auto REPLY_017{ std::to_array("Thanks for coming to meet me personally.\033S") };
inline constexpr auto REPLY_018{ std::to_array("What is the status of your forces, Commander %s?\033S") };
inline constexpr auto REPLY_019{ std::to_array("It doesn't seem too\n"
                                               "bad.\033S") };
inline constexpr auto REPLY_020{ std::to_array("I have every confidence\n"
                                               "that will complete your\n"
                                               "duties successfully.\033S") };
inline constexpr auto REPLY_021{ std::to_array("") };
inline constexpr auto REPLY_022{ std::to_array("I'll be at Army HQ, waiting for some positive news.\033S") };
inline constexpr auto REPLY_023{ std::to_array("Commander %s! What are you waiting for?!\033S") };
inline constexpr auto REPLY_024{ std::to_array("When I heard you were having trouble, I came right over;\033S") };
inline constexpr auto REPLY_025{ std::to_array("but these guys should take you no time at\n"
                                               "all!\033S") };
inline constexpr auto REPLY_026{ std::to_array("You'd better finish these guys off quick! "
                                               "And that's an order, mister!\033S") };
inline constexpr auto REPLY_027{ std::to_array("A bridge was wiped out by the %s.\033S") };
inline constexpr auto REPLY_028{ std::to_array("The %s damaged\n"
                                               "the defenses in\n"
                                               "%s!\033S") };
inline constexpr auto REPLY_029{ std::to_array("\033C7%s forces raided \033C6Axis %s\033C7.\033S") };
inline constexpr auto REPLY_030{ std::to_array("We've been attacked by %s forces!\033S") };
inline constexpr auto REPLY_031{ std::to_array("Our battleships hit the \033C6Axis %s\033C7!\033S") };
inline constexpr auto REPLY_032{ std::to_array("We've been hit by the \033C5Allied battleships\033C7!\033S") };
inline constexpr auto REPLY_033{ std::to_array("Fire the V-1 rockets!\033S") };
inline constexpr auto REPLY_034{ std::to_array("The V-1 rockets hit\n"
                                               "\033C5%s\033C7!\033S") };
inline constexpr auto REPLY_035{ std::to_array("V-1 rockets approaching, sir!\033S") };
inline constexpr auto REPLY_036{ std::to_array("\033C5%s\033C7\n"
                                               "has been hit by the\n"
                                               "German V-1 rockets!\033S") };
inline constexpr auto REPLY_037{ std::to_array("War benefits no one.\n"
                                               "The Church wants this\n"
                                               "war to end.\033S") };
inline constexpr auto REPLY_038{ std::to_array("Please, in the\n"
                                               "name of God, stop\n"
                                               "this madness.\033S") };
inline constexpr auto REPLY_039{ std::to_array("Everyone is equal\n"
                                               "in the eyes of the\n"
                                               "Lord.\033S") };
inline constexpr auto REPLY_040{ std::to_array("There are no enemies or\n"
                                               "allies; we are all\n"
                                               "brothers and sisters.\033S") };
inline constexpr auto REPLY_041{ std::to_array("Please let me tend\n"
                                               "to the wounded.\033S") };
inline constexpr auto REPLY_042{ std::to_array("The tyranny of\n"
                                               "dictators will be\n"
                                               "judged by God!\033S") };
inline constexpr auto REPLY_043{ std::to_array("\033M4\033C6Village Girl\033M0\033C7") };
inline constexpr auto REPLY_044{ std::to_array("It's nice to see you\n"
                                               "again Commandant\n"
                                               "%s.\033S") };
inline constexpr auto REPLY_045{ std::to_array("Do you remember me?\n"
                                               "I gave you flowers\n"
                                               "when you went marching\n"
                                               "to battle.\033S") };
inline constexpr auto REPLY_046{ std::to_array("It's been 4 years\n"
                                               "since that day, and\n"
                                               "I just wanted to\n"
                                               "see you again.\033S") };
inline constexpr auto REPLY_047{ std::to_array("I pleaded with your\n"
                                               "staff and they\n"
                                               "finally let me in.\033S") };
inline constexpr auto REPLY_048{ std::to_array("I've brought some tea\n"
                                               "and homemade pie\n"
                                               "for everyone.\033S") };
inline constexpr auto REPLY_049{ std::to_array("What a wonderful day!\033S") };
inline constexpr auto REPLY_050{ std::to_array("I suppose staying any\n"
                                               "longer is out of the\n"
                                               "question. I don't want\n"
                                               "to be a bother.\033S") };
inline constexpr auto REPLY_051{ std::to_array("I hope to see you again\n"
                                               "someday.\n"
                                               "Goodbye now...\033S") };
inline constexpr auto REPLY_052{ std::to_array("I know how you must be feeling; the destruction, the\n") };
inline constexpr auto REPLY_053{ std::to_array("killing...The terrible face of war has been emblazoned on\n") };
inline constexpr auto REPLY_054{ std::to_array("your soul, and I doubt you'll ever forget it.") };
inline constexpr auto REPLY_055{ std::to_array("over soon, but until then Commander, your country is\n") };
inline constexpr auto REPLY_056{ std::to_array("There's a girl here to see you, Commandant. She says she's brought you "
                                               "some tea.\033S") };
inline constexpr auto REPLY_057{ std::to_array("Will you see her?") };
inline constexpr auto REPLY_058{ std::to_array("That girl is back again.\n"
                                               "Should we let her in,\n"
                                               "Commandant?") };
inline constexpr auto REPLY_059{ std::to_array("The troops are exhausted.\033S") };
inline constexpr auto REPLY_060{ std::to_array("We're out of fuel.\033S") };
inline constexpr auto REPLY_061{ std::to_array("%s is under enemy control. We cannot get %s to you.\033S") };
inline constexpr auto REPLY_062{ std::to_array("Your forces are isolated. We cannot get %s to you.\033S") };
inline constexpr auto REPLY_063{ std::to_array("I'm sorry, but our plan to send you %s had to be aborted.\033S") };
inline constexpr auto REPLY_064{ std::to_array("Your request for %s has been approved. They will be sent soon.\033S") };
inline constexpr auto REPLY_065{ std::to_array("They have decided that you can continue to fight without %s.") };
inline constexpr auto REPLY_066{ std::to_array("After an evaluation, they decided to turn down your request for %s.\033S") };
inline constexpr auto REPLY_067{ std::to_array("Their evaluation states that your request for %s be denied.\033S") };
inline constexpr auto REPLY_068{ std::to_array("They do not see the need for additional\n"
                                               "%s.\033S") };
inline constexpr auto REPLY_069{ std::to_array("Your request for %s has been turned down.\033S") };
inline constexpr auto REPLY_070{ std::to_array("We are preparing for a large-scale "
                                               "offensive. We cannot spare any %s.\033S") };
inline constexpr auto REPLY_071{ std::to_array("They can't send\n"
                                               "additional %s.\n"
                                               "They asked you to try\n"
                                               "and manage.\033S") };
inline constexpr auto REPLY_072{ std::to_array("Supplies arrived in \033C%c%s\033C7.\033S") };
inline constexpr auto REPLY_073{ std::to_array("Fuel %u(+%u)\n"
                                               "Ammo %u(+%u)\n"
                                               "Food %u(+%u)\033S") };
inline constexpr auto REPLY_074{ std::to_array("A supplement of new troops has arrived in %s.\033S") };
inline constexpr auto REPLY_075{ std::to_array("The \033C%c%s \033C%c%s\033C7 has arrived!\033S") };
inline constexpr auto REPLY_076{ std::to_array("You've won today's battle, but the war rages on in other\n") };
inline constexpr auto REPLY_077{ std::to_array("parts of the world. We need you and your brave men,\n") };
inline constexpr auto REPLY_078{ std::to_array("Commander. Their courage and fighting spirit, under\n") };
inline constexpr auto REPLY_079{ std::to_array("your leadership, could put an end to this whole mess.") };
inline constexpr auto REPLY_080{ std::to_array("Without control of the skies, we can't make a large-scale air raid.\033S") };
inline constexpr auto REPLY_081{ std::to_array("With the number of bombers we have now, we can't make a very effective "
                                               "bombing run.\033S") };
inline constexpr auto REPLY_082{ std::to_array("We don't have any bombers you can use at this time.\033S") };
inline constexpr auto REPLY_083{ std::to_array("Carpet bombing over enemy units is a waste of planes.\033S") };
inline constexpr auto REPLY_084{ std::to_array("Unfortunately, this is not a good time.\033S") };
inline constexpr auto REPLY_085{ std::to_array("Without command of the skies, we can't afford to send out too many of our "
                                               "bombers.\033S") };
inline constexpr auto REPLY_086{ std::to_array("We cannot provide effective air support with the number of bombers we "
                                               "have now.\033S") };
inline constexpr auto REPLY_087{ std::to_array("A strafing attack should stop the enemy units.\033S") };
inline constexpr auto REPLY_088{ std::to_array("Due to a lack of bombers, we can't make an effective city bombing.\033S") };
inline constexpr auto REPLY_089{ std::to_array("Our ground forces shouldn't expect too much air support when we attack "
                                               "the city.\033S") };
inline constexpr auto REPLY_090{ std::to_array("Are you telling me that I should just send my pilots out to die?!") };
inline constexpr auto REPLY_091{ std::to_array("With this few bombers, we will never be able to give our ground forces "
                                               "support.\033S") };
inline constexpr auto REPLY_092{ std::to_array("It's very dangerous to attack a city without control of the skies.\033S") };
inline constexpr auto REPLY_093{ std::to_array("How do you propose we attack the city "
                                               "with the number of bombers we have now?!") };
inline constexpr auto REPLY_094{ std::to_array("Machine gunfire is very ineffective against cities.\033S") };
inline constexpr auto REPLY_095{ std::to_array("In %d %s, %d %s\n"
                                               "will execute\n"
                                               "%s operation.\033S") };
inline constexpr auto REPLY_096{ std::to_array("Call off the\n"
                                               "air mission, sir?") };
inline constexpr auto REPLY_097{ std::to_array("There are no aircraft ready for action.\033E2") };
inline constexpr auto REPLY_098{ std::to_array("We will ask Airforce HQ\n"
                                               "for an air strike.\n"
                                               "Are these your orders?") };
inline constexpr auto REPLY_099{ std::to_array("depending on you. ") };
inline constexpr auto REPLY_100{ std::to_array("They are presently executing a mission, sir.\033W2") };
inline constexpr auto REPLY_101{ std::to_array(" had their communications cut!") };
inline constexpr auto REPLY_102{ std::to_array("You cannot drop %s over a %s.\033W3") };
inline constexpr auto REPLY_103{ std::to_array("Orders understood.\n"
                                               "Proceed?") };
inline constexpr auto REPLY_104{ std::to_array("What scale of air raid\n"
                                               "do you want, sir") };
inline constexpr auto REPLY_105{ std::to_array("It's too dangerous to\n"
                                               "send paratroopers in\n"
                                               "the daytime.\033S") };
inline constexpr auto REPLY_106{ std::to_array("mission to cut comm of ") };
inline constexpr auto REPLY_107{ std::to_array("I must be the happiest\n"
                                               "girl in the world!\033S") };
inline constexpr auto REPLY_108{ std::to_array("Even if I were to die\n"
                                               "today, I will never\n"
                                               "regret the time spent\n"
                                               "with you.\033S") };
inline constexpr auto REPLY_109{ std::to_array("A strong %s has started, Commander!") };
inline constexpr auto REPLY_110{ std::to_array("We cannot do mining\n"
                                               "operations there.\033E2") };
inline constexpr auto REPLY_111{ std::to_array("Understood.\n"
                                               "I'll have them build a bridge.\033W1") };
inline constexpr auto REPLY_112{ std::to_array("Understood.\n"
                                               "We'll remove the minefield.\033W1") };
inline constexpr auto REPLY_113{ std::to_array("Understood.\n"
                                               "We'll destroy the bridge.\033W1") };
inline constexpr auto REPLY_114{ std::to_array("Preparations will take\n"
                                               "at least %d hours.\033S") };
inline constexpr auto REPLY_115{ std::to_array("Preparation usually takes %d hours but, "
                                               "we'll try to shorten it for you.\033S") };
inline constexpr auto REPLY_116{ std::to_array("Transporting cargo in the daytime is too dangerous!\033S") };
inline constexpr auto REPLY_117{ std::to_array("How long should we wait, sir(1-24)") };
inline constexpr auto REPLY_118{ std::to_array("How long should we wait, sir") };
inline constexpr auto REPLY_119{ std::to_array("We'll wait 1 hour.\n"
                                               "Is this correct, sir?") };
inline constexpr auto REPLY_120{ std::to_array("%2s   %2d    %2d    %2d    %2d    %2d    %2d    %2d    %2d  %-4.4s  %-4s") };
inline constexpr auto REPLY_121{ std::to_array("In %d hours, %d %s will drop our paratroops.\033S") };
inline constexpr auto REPLY_122{ std::to_array("Nat  Ldr  Chrm  Brvy   Hlt   Inf  Arty  Tanks  Exp  Cmd   Stat") };
inline constexpr auto REPLY_123{ std::to_array("   ------      ------    ----   ----   ----   ---- ") };
inline constexpr auto REPLY_124{ std::to_array("Understood, sir.\n"
                                               "It will take about %d hours.\033S") };

} // namespace opeu::game::detail::freply

namespace opeu::game {
using detail::freply::REPLY_000;
using detail::freply::REPLY_001;
using detail::freply::REPLY_002;
using detail::freply::REPLY_003;
using detail::freply::REPLY_004;
using detail::freply::REPLY_005;
using detail::freply::REPLY_006;
using detail::freply::REPLY_007;
using detail::freply::REPLY_008;
using detail::freply::REPLY_009;
using detail::freply::REPLY_010;
using detail::freply::REPLY_011;
using detail::freply::REPLY_012;
using detail::freply::REPLY_013;
using detail::freply::REPLY_014;
using detail::freply::REPLY_015;
using detail::freply::REPLY_016;
using detail::freply::REPLY_017;
using detail::freply::REPLY_018;
using detail::freply::REPLY_019;
using detail::freply::REPLY_020;
using detail::freply::REPLY_021;
using detail::freply::REPLY_022;
using detail::freply::REPLY_023;
using detail::freply::REPLY_024;
using detail::freply::REPLY_025;
using detail::freply::REPLY_026;
using detail::freply::REPLY_027;
using detail::freply::REPLY_028;
using detail::freply::REPLY_029;
using detail::freply::REPLY_030;
using detail::freply::REPLY_031;
using detail::freply::REPLY_032;
using detail::freply::REPLY_033;
using detail::freply::REPLY_034;
using detail::freply::REPLY_035;
using detail::freply::REPLY_036;
using detail::freply::REPLY_037;
using detail::freply::REPLY_038;
using detail::freply::REPLY_039;
using detail::freply::REPLY_040;
using detail::freply::REPLY_041;
using detail::freply::REPLY_042;
using detail::freply::REPLY_043;
using detail::freply::REPLY_044;
using detail::freply::REPLY_045;
using detail::freply::REPLY_046;
using detail::freply::REPLY_047;
using detail::freply::REPLY_048;
using detail::freply::REPLY_049;
using detail::freply::REPLY_050;
using detail::freply::REPLY_051;
using detail::freply::REPLY_052;
using detail::freply::REPLY_053;
using detail::freply::REPLY_054;
using detail::freply::REPLY_055;
using detail::freply::REPLY_056;
using detail::freply::REPLY_057;
using detail::freply::REPLY_058;
using detail::freply::REPLY_059;
using detail::freply::REPLY_060;
using detail::freply::REPLY_061;
using detail::freply::REPLY_062;
using detail::freply::REPLY_063;
using detail::freply::REPLY_064;
using detail::freply::REPLY_065;
using detail::freply::REPLY_066;
using detail::freply::REPLY_067;
using detail::freply::REPLY_068;
using detail::freply::REPLY_069;
using detail::freply::REPLY_070;
using detail::freply::REPLY_071;
using detail::freply::REPLY_072;
using detail::freply::REPLY_073;
using detail::freply::REPLY_074;
using detail::freply::REPLY_075;
using detail::freply::REPLY_076;
using detail::freply::REPLY_077;
using detail::freply::REPLY_078;
using detail::freply::REPLY_079;
using detail::freply::REPLY_080;
using detail::freply::REPLY_081;
using detail::freply::REPLY_082;
using detail::freply::REPLY_083;
using detail::freply::REPLY_084;
using detail::freply::REPLY_085;
using detail::freply::REPLY_086;
using detail::freply::REPLY_087;
using detail::freply::REPLY_088;
using detail::freply::REPLY_089;
using detail::freply::REPLY_090;
using detail::freply::REPLY_091;
using detail::freply::REPLY_092;
using detail::freply::REPLY_093;
using detail::freply::REPLY_094;
using detail::freply::REPLY_095;
using detail::freply::REPLY_096;
using detail::freply::REPLY_097;
using detail::freply::REPLY_098;
using detail::freply::REPLY_099;
using detail::freply::REPLY_100;
using detail::freply::REPLY_101;
using detail::freply::REPLY_102;
using detail::freply::REPLY_103;
using detail::freply::REPLY_104;
using detail::freply::REPLY_105;
using detail::freply::REPLY_106;
using detail::freply::REPLY_107;
using detail::freply::REPLY_108;
using detail::freply::REPLY_109;
using detail::freply::REPLY_110;
using detail::freply::REPLY_111;
using detail::freply::REPLY_112;
using detail::freply::REPLY_113;
using detail::freply::REPLY_114;
using detail::freply::REPLY_115;
using detail::freply::REPLY_116;
using detail::freply::REPLY_117;
using detail::freply::REPLY_118;
using detail::freply::REPLY_119;
using detail::freply::REPLY_120;
using detail::freply::REPLY_121;
using detail::freply::REPLY_122;
using detail::freply::REPLY_123;
using detail::freply::REPLY_124;
} // namespace opeu::game
