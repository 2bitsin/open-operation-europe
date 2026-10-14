// Morale reports and dated war-news messages, selected through the data segment's far pointer table.
#pragma once

#include <array>
#include <cstdint>

namespace opeu::game::detail::fnews {

inline constexpr auto WAR_NEWS_000{ std::to_array("Morale for the entire\n"
                                                  "army is extremely\n"
                                                  "high.") };
inline constexpr auto WAR_NEWS_001{ std::to_array("Stop the approaching\n"
                                                  "German troops at\n"
                                                  "Dunkirk.") };
inline constexpr auto WAR_NEWS_002{ std::to_array("No more troops will\n"
                                                  "be approved for\n"
                                                  "the attack on Dunkirk.") };
inline constexpr auto WAR_NEWS_003{ std::to_array("It's dangerous to\n"
                                                  "attack Dunkirk\n"
                                                  "without armored units.") };
inline constexpr auto WAR_NEWS_004{ std::to_array("Let's escape to England\n"
                                                  "while the Germans are\n"
                                                  "in disarray.") };
inline constexpr auto WAR_NEWS_005{ std::to_array("Since Belgium has\n"
                                                  "surrendered, we can\n"
                                                  "resume supplying arms.") };
inline constexpr auto WAR_NEWS_006{ std::to_array("Withdrawal from\n"
                                                  "Dunkirk was completed\n"
                                                  "safely.") };
inline constexpr auto WAR_NEWS_007{ std::to_array("Aren't our armored\n"
                                                  "units still able to\n"
                                                  "break through enemy\n"
                                                  "lines?") };
inline constexpr auto WAR_NEWS_008{ std::to_array("Our comrade Mussolini\n"
                                                  "decided to enter\n"
                                                  "the war!") };
inline constexpr auto WAR_NEWS_009{ std::to_array("The enemy is in Paris!\n"
                                                  "Withdraw our troops\n"
                                                  "from the Maginot Line!") };
inline constexpr auto WAR_NEWS_010{ std::to_array("Foul, cursed Germans!\n"
                                                  "We will liberate France\n"
                                                  "by any means!") };
inline constexpr auto WAR_NEWS_011{ std::to_array("The Allies have taken\n"
                                                  "command over the air\n"
                                                  "of Europe!") };
inline constexpr auto WAR_NEWS_012{ std::to_array("The time for strategic\n"
                                                  "bombers has come.") };
inline constexpr auto WAR_NEWS_013{ std::to_array("Japan no longer\n"
                                                  "controls the Pacific.") };
inline constexpr auto WAR_NEWS_014{ std::to_array("We're doing well on\n"
                                                  "the African front.\n"
                                                  "Malta will fall soon.") };
inline constexpr auto WAR_NEWS_015{ std::to_array("We must make up for\n"
                                                  "our loss during the\n"
                                                  "winter offensive in\n"
                                                  "Russia.") };
inline constexpr auto WAR_NEWS_016{ std::to_array("Our army is making\n"
                                                  "a great advance to\n"
                                                  "the south!") };
inline constexpr auto WAR_NEWS_017{ std::to_array("It's a disgrace to\n"
                                                  "have let the Allied\n"
                                                  "forces land in Italy!") };
inline constexpr auto WAR_NEWS_018{ std::to_array("The Germans must\n"
                                                  "first be repelled\n"
                                                  "from north Kursk!") };
inline constexpr auto WAR_NEWS_019{ std::to_array("All our reinforce-\n"
                                                  "ments are being sent\n"
                                                  "to southern Europe.") };
inline constexpr auto WAR_NEWS_020{ std::to_array("What's happened to\n"
                                                  "Mussolini?!") };
inline constexpr auto WAR_NEWS_021{ std::to_array("We no longer need\n"
                                                  "to worry about a\n"
                                                  "pincer operation at\n"
                                                  "Kursk.") };
inline constexpr auto WAR_NEWS_022{ std::to_array("At this point,\n"
                                                  "Germany's downfall\n"
                                                  "is inevitable.") };
inline constexpr auto WAR_NEWS_023{ std::to_array("We captured Volkhov.\n"
                                                  "Now the Germans are\n"
                                                  "surrounded at Kursk.") };
inline constexpr auto WAR_NEWS_024{ std::to_array("Now that Volkhov has\n"
                                                  "fallen, supplies and\n"
                                                  "arms cannot be\n"
                                                  "delivered.") };
inline constexpr auto WAR_NEWS_025{ std::to_array("We can't count on\n"
                                                  "Italy without\n"
                                                  "Mussolini.") };
inline constexpr auto WAR_NEWS_026{ std::to_array("Terrorize London\n"
                                                  "with V-1 rockets!") };
inline constexpr auto WAR_NEWS_027{ std::to_array("Carpet bomb the\n"
                                                  "German military\n"
                                                  "facilities!") };
inline constexpr auto WAR_NEWS_028{ std::to_array("The Allies have\n"
                                                  "decided to create\n"
                                                  "both an Eastern and\n"
                                                  "Western front.") };
inline constexpr auto WAR_NEWS_029{ std::to_array("Allied forces have\n"
                                                  "gained total control\n"
                                                  "over the Cotentin\n"
                                                  "peninsula.") };
inline constexpr auto WAR_NEWS_030{ std::to_array("We have withdrawn from\n"
                                                  "Russia. Our front line\n"
                                                  "has grown too thin.") };
inline constexpr auto WAR_NEWS_031{ std::to_array("How dare they attempt\n"
                                                  "to assassinate me!\n"
                                                  "They'll pay for this!") };
inline constexpr auto WAR_NEWS_032{ std::to_array("We had air support\n"
                                                  "for the operation.") };
inline constexpr auto WAR_NEWS_033{ std::to_array("Now we can pound\n"
                                                  "the Germans at\n"
                                                  "Normandy!") };
inline constexpr auto WAR_NEWS_034{ std::to_array("Don't be ridiculous!\n"
                                                  "Russia just stood\n"
                                                  "by and watched\n"
                                                  "Warsaw get invaded.") };
inline constexpr auto WAR_NEWS_035{ std::to_array("Our plan to retake\n"
                                                  "Avranches failed. We\n"
                                                  "can no longer expect\n"
                                                  "reinforcements.") };
inline constexpr auto WAR_NEWS_036{ std::to_array("It's become very\n"
                                                  "difficult to supply\n"
                                                  "materials and arms\n"
                                                  "to Normandy.") };
inline constexpr auto WAR_NEWS_037{ std::to_array("Launch\n"
                                                  "Operation Dragoon!") };
inline constexpr auto WAR_NEWS_038{ std::to_array("Southern France is\n"
                                                  "weak. We shouldn't\n"
                                                  "count on their help\n"
                                                  "on the front line.") };
inline constexpr auto WAR_NEWS_039{ std::to_array("With the fall of St.\n"
                                                  "Vith, defense of\n"
                                                  "Bastogne has become\n"
                                                  "even more crucial.") };
inline constexpr auto WAR_NEWS_040{ std::to_array("If we break through\n"
                                                  "at Bastogne, the\n"
                                                  "enemy won't have a\n"
                                                  "chance!") };
inline constexpr auto WAR_NEWS_041{ std::to_array("Russian forces have\n"
                                                  "restarted their\n"
                                                  "advance!") };
inline constexpr auto WAR_NEWS_042{ std::to_array("We must solidify our\n"
                                                  "defense in Berlin!") };
inline constexpr auto WAR_NEWS_043{ std::to_array("The Western Front has\n"
                                                  "finally collapsed...") };
inline constexpr auto WAR_NEWS_044{ std::to_array("We, the Russians,\n"
                                                  "will hold the honor of\n"
                                                  "occupying Berlin.") };
inline constexpr auto WAR_NEWS_045{ std::to_array("You haven't beaten me\n"
                                                  "yet!") };
inline constexpr auto WAR_NEWS_046{ std::to_array("Defending Berlin has\n"
                                                  "taken priority. Forces\n"
                                                  "cannot be sent\n"
                                                  "elsewhere.") };
inline constexpr auto WAR_NEWS_047{ std::to_array("Mussolini is dead.\n"
                                                  "I'll be damned if\n"
                                                  "I go like that.") };
inline constexpr auto WAR_NEWS_048{ std::to_array("The enemy has broken\n"
                                                  "through to Berlin. We\n"
                                                  "can no longer send you\n"
                                                  "reinforcements.") };
inline constexpr auto WAR_NEWS_049{ std::to_array("Germans have started an all-out attack on the Western Front.") };
inline constexpr auto WAR_NEWS_050{ std::to_array("Holland surrendered! Germany is out for all of Europe!") };
inline constexpr auto WAR_NEWS_051{ std::to_array("German forces have occupied Brussels.") };
inline constexpr auto WAR_NEWS_052{ std::to_array("Allied forces have counter-attacked at Dunkirk.") };
inline constexpr auto WAR_NEWS_053{ std::to_array("The German advance was halted!") };
inline constexpr auto WAR_NEWS_054{ std::to_array("British-French forces have begun to withdraw from Dunkirk.") };
inline constexpr auto WAR_NEWS_055{ std::to_array("Belgium and Leopold III have surrendered.") };
inline constexpr auto WAR_NEWS_056{ std::to_array("British-French forces have completely withdrawn from Dunkirk.") };
inline constexpr auto WAR_NEWS_057{ std::to_array("German forces have launched a massive offensive into Somme.") };
inline constexpr auto WAR_NEWS_058{ std::to_array("Norway surrendered. Italy has declared war on Britain and France.") };
inline constexpr auto WAR_NEWS_059{ std::to_array("Allied command has ordered defending "
                                                  "units on the Maginot Line to withdraw.") };
inline constexpr auto WAR_NEWS_060{ std::to_array("German forces have forced their way into Paris.") };
inline constexpr auto WAR_NEWS_061{ std::to_array("The 'Free France' movement was established.") };
inline constexpr auto WAR_NEWS_062{ std::to_array("France signed a cease-fire agreement with Germany.") };
inline constexpr auto WAR_NEWS_063{ std::to_array("German and Italian forces have started an offensive into northern Africa.") };
inline constexpr auto WAR_NEWS_064{ std::to_array("Mexico has declared war on the Axis powers.") };
inline constexpr auto WAR_NEWS_065{ std::to_array("British forces air raided Klin. The Russian counter-attack at Volkhov was "
                                                  "a failure.") };
inline constexpr auto WAR_NEWS_066{ std::to_array("The Royal British air force executed mass raids over Essen.") };
inline constexpr auto WAR_NEWS_067{ std::to_array("The Japanese navy was defeated at Midway.") };
inline constexpr auto WAR_NEWS_068{ std::to_array("Japanese troops began landing on the Aleutian Islands.") };
inline constexpr auto WAR_NEWS_069{ std::to_array("US-Russia sign Mutual Aid Agreement.") };
inline constexpr auto WAR_NEWS_070{ std::to_array("German-Italian forces blocked a British landing on Malta.") };
inline constexpr auto WAR_NEWS_071{ std::to_array("The Japanese have landed at Guadalcanal.") };
inline constexpr auto WAR_NEWS_072{ std::to_array("Britian-Russia sign Mutual Aid Agreement.") };
inline constexpr auto WAR_NEWS_073{ std::to_array("German forces have begun a summer offensive against Russia.") };
inline constexpr auto WAR_NEWS_074{ std::to_array("German troops have occupied Sebastopol.") };
inline constexpr auto WAR_NEWS_075{ std::to_array("German forces ordered to cancel the Tobruk offensive.") };
inline constexpr auto WAR_NEWS_076{ std::to_array("German troops took the offensive in the Kursk area.") };
inline constexpr auto WAR_NEWS_077{ std::to_array("Allies began landing on Sicily.") };
inline constexpr auto WAR_NEWS_078{ std::to_array("Russian forces have made a massive offensive into Briansk in northern "
                                                  "Kursk.") };
inline constexpr auto WAR_NEWS_079{ std::to_array("Allied forces have occupied Palermo.") };
inline constexpr auto WAR_NEWS_080{ std::to_array("US-British forces carried out heavy air raids on Hamburg.") };
inline constexpr auto WAR_NEWS_081{ std::to_array("In Italy, the Fascist Regime collapsed, and Mussolini was arrested.") };
inline constexpr auto WAR_NEWS_082{ std::to_array("New Italian Prime Minister Badoglio declared that Italy will stay in the "
                                                  "war.") };
inline constexpr auto WAR_NEWS_083{ std::to_array("Japanese troops withdrew from the Aleutian Islands.") };
inline constexpr auto WAR_NEWS_084{ std::to_array("Burma has declared its independence.") };
inline constexpr auto WAR_NEWS_085{ std::to_array("Russian troops retook Orel. German troops in northern Kursk began to "
                                                  "withdraw.") };
inline constexpr auto WAR_NEWS_086{ std::to_array("German-Italian forces withdrew from Sicily. Russian troops recaptured "
                                                  "Briansk.") };
inline constexpr auto WAR_NEWS_087{ std::to_array("German troops withdrew from Volkhov.") };
inline constexpr auto WAR_NEWS_088{ std::to_array("Rome announced its inability to defend itself.") };
inline constexpr auto WAR_NEWS_089{ std::to_array("Japanese forces were defeated on New Georgia Island.") };
inline constexpr auto WAR_NEWS_090{ std::to_array("Italy has signed an armistice with the Allies.") };
inline constexpr auto WAR_NEWS_091{ std::to_array("German forces ordered to cancel Kursk offensive.") };
inline constexpr auto WAR_NEWS_092{ std::to_array("Allied forces began landing in Normandy.") };
inline constexpr auto WAR_NEWS_093{ std::to_array("German forces began launching V-1 rockets.") };
inline constexpr auto WAR_NEWS_094{ std::to_array("US forces landed on Saipan.") };
inline constexpr auto WAR_NEWS_095{ std::to_array("Japanese forces were defeated in a battle for the Marianas Islands.") };
inline constexpr auto WAR_NEWS_096{ std::to_array("French troops occupied Elba Island.") };
inline constexpr auto WAR_NEWS_097{ std::to_array("Allied forces bombed industrial facilities near Berlin.") };
inline constexpr auto WAR_NEWS_098{ std::to_array("Russian forces started a massive offensive into White Russia.") };
inline constexpr auto WAR_NEWS_099{ std::to_array("Fortified city of Cherbourg has capitulated.") };
inline constexpr auto WAR_NEWS_100{ std::to_array("Japanese forces were crushed in Saipan.") };
inline constexpr auto WAR_NEWS_101{ std::to_array("Russian forces have liberated Minsk.") };
inline constexpr auto WAR_NEWS_102{ std::to_array("Italian government will return to Rome.") };
inline constexpr auto WAR_NEWS_103{ std::to_array("Everyone in the Japanese Tojo cabinet has resigned.") };
inline constexpr auto WAR_NEWS_104{ std::to_array("Hitler survived an assassination attempt. Russian forces have occupied "
                                                  "Luberlin.") };
inline constexpr auto WAR_NEWS_105{ std::to_array("Russian forces have occupied Luberlin.") };
inline constexpr auto WAR_NEWS_106{ std::to_array("US forces have occupied Pisa.") };
inline constexpr auto WAR_NEWS_107{ std::to_array("Allied forces have occupied Avranches.") };
inline constexpr auto WAR_NEWS_108{ std::to_array("There was an uprising in Warsaw.") };
inline constexpr auto WAR_NEWS_109{ std::to_array("German forces have withdrawn from Finland.") };
inline constexpr auto WAR_NEWS_110{ std::to_array("The German counter-attack at Avranches has failed.") };
inline constexpr auto WAR_NEWS_111{ std::to_array("Allied forces have occupied Le Mans.") };
inline constexpr auto WAR_NEWS_112{ std::to_array("Japanese troops in Guam were destroyed.") };
inline constexpr auto WAR_NEWS_113{ std::to_array("Allied forces have landed in southern France.") };
inline constexpr auto WAR_NEWS_114{ std::to_array("German troops in southern France have been ordered to pull-back.") };
inline constexpr auto WAR_NEWS_115{ std::to_array("Uprising in Paris. German troops were ordered to withdraw from Normandy.") };
inline constexpr auto WAR_NEWS_116{ std::to_array("German forces began a counter-offensive in the Ardennes.") };
inline constexpr auto WAR_NEWS_117{ std::to_array("Japanese forces in Leyte were destroyed.") };
inline constexpr auto WAR_NEWS_118{ std::to_array("German troops occupied St. Vith.") };
inline constexpr auto WAR_NEWS_119{ std::to_array("Allied forces counter-attack near Dinan. German advance halted.") };
inline constexpr auto WAR_NEWS_120{ std::to_array("German forces have launched a counter-offensive at Strausberg.") };
inline constexpr auto WAR_NEWS_121{ std::to_array("Greek National Liberation Front has signed a cease-fire agreement with "
                                                  "Britain.") };
inline constexpr auto WAR_NEWS_122{ std::to_array("Russian forces began a massive offensive on the Eastern Front.") };
inline constexpr auto WAR_NEWS_123{ std::to_array("Russian forces have attacked Eastern Prussia.") };
inline constexpr auto WAR_NEWS_124{ std::to_array("Russian forces have occupied Warsaw.") };
inline constexpr auto WAR_NEWS_125{ std::to_array("Polish provisional government has moved to Warsaw.") };
inline constexpr auto WAR_NEWS_126{ std::to_array("German command has ordered a stop to the attack on the Ardennes.") };
inline constexpr auto WAR_NEWS_127{ std::to_array("Russian forces have crossed the Nice River.") };
inline constexpr auto WAR_NEWS_128{ std::to_array("German troops have surrendered at the Ruhr River.") };
inline constexpr auto WAR_NEWS_129{ std::to_array("US forces were stopped before advancing into Prague.") };
inline constexpr auto WAR_NEWS_130{ std::to_array("Allied forces have crossed the Po River and invaded Northern Italy "
                                                  "(Trento area).") };
inline constexpr auto WAR_NEWS_131{ std::to_array("Hitler celebrated his 57th birthday.") };
inline constexpr auto WAR_NEWS_132{ std::to_array("Allied forces have freed Stuttgart.") };
inline constexpr auto WAR_NEWS_133{ std::to_array("US and Soviet forces have joined together at the Elbe River.") };
inline constexpr auto WAR_NEWS_134{ std::to_array("Mussolini has been executed.") };
inline constexpr auto WAR_NEWS_135{ std::to_array("Allied forces have occupied Turin.") };
inline constexpr auto WAR_NEWS_136{ std::to_array("The war in Burma has ended.") };
inline constexpr auto WAR_NEWS_137{ std::to_array("They're rioting in Prague!") };
inline constexpr auto WAR_NEWS_138{ std::to_array("US-British forces have entered Berlin.") };
inline constexpr auto WAR_NEWS_139{ std::to_array("German troops in Bohemia have surrendered.") };
inline constexpr auto WAR_NEWS_140{ std::to_array("Hitler has committed suicide.") };
inline constexpr auto WAR_NEWS_141{ std::to_array("Reinforcements will arrive soon. Try to hold on.") };

} // namespace opeu::game::detail::fnews

namespace opeu::game {
using detail::fnews::WAR_NEWS_000;
using detail::fnews::WAR_NEWS_001;
using detail::fnews::WAR_NEWS_002;
using detail::fnews::WAR_NEWS_003;
using detail::fnews::WAR_NEWS_004;
using detail::fnews::WAR_NEWS_005;
using detail::fnews::WAR_NEWS_006;
using detail::fnews::WAR_NEWS_007;
using detail::fnews::WAR_NEWS_008;
using detail::fnews::WAR_NEWS_009;
using detail::fnews::WAR_NEWS_010;
using detail::fnews::WAR_NEWS_011;
using detail::fnews::WAR_NEWS_012;
using detail::fnews::WAR_NEWS_013;
using detail::fnews::WAR_NEWS_014;
using detail::fnews::WAR_NEWS_015;
using detail::fnews::WAR_NEWS_016;
using detail::fnews::WAR_NEWS_017;
using detail::fnews::WAR_NEWS_018;
using detail::fnews::WAR_NEWS_019;
using detail::fnews::WAR_NEWS_020;
using detail::fnews::WAR_NEWS_021;
using detail::fnews::WAR_NEWS_022;
using detail::fnews::WAR_NEWS_023;
using detail::fnews::WAR_NEWS_024;
using detail::fnews::WAR_NEWS_025;
using detail::fnews::WAR_NEWS_026;
using detail::fnews::WAR_NEWS_027;
using detail::fnews::WAR_NEWS_028;
using detail::fnews::WAR_NEWS_029;
using detail::fnews::WAR_NEWS_030;
using detail::fnews::WAR_NEWS_031;
using detail::fnews::WAR_NEWS_032;
using detail::fnews::WAR_NEWS_033;
using detail::fnews::WAR_NEWS_034;
using detail::fnews::WAR_NEWS_035;
using detail::fnews::WAR_NEWS_036;
using detail::fnews::WAR_NEWS_037;
using detail::fnews::WAR_NEWS_038;
using detail::fnews::WAR_NEWS_039;
using detail::fnews::WAR_NEWS_040;
using detail::fnews::WAR_NEWS_041;
using detail::fnews::WAR_NEWS_042;
using detail::fnews::WAR_NEWS_043;
using detail::fnews::WAR_NEWS_044;
using detail::fnews::WAR_NEWS_045;
using detail::fnews::WAR_NEWS_046;
using detail::fnews::WAR_NEWS_047;
using detail::fnews::WAR_NEWS_048;
using detail::fnews::WAR_NEWS_049;
using detail::fnews::WAR_NEWS_050;
using detail::fnews::WAR_NEWS_051;
using detail::fnews::WAR_NEWS_052;
using detail::fnews::WAR_NEWS_053;
using detail::fnews::WAR_NEWS_054;
using detail::fnews::WAR_NEWS_055;
using detail::fnews::WAR_NEWS_056;
using detail::fnews::WAR_NEWS_057;
using detail::fnews::WAR_NEWS_058;
using detail::fnews::WAR_NEWS_059;
using detail::fnews::WAR_NEWS_060;
using detail::fnews::WAR_NEWS_061;
using detail::fnews::WAR_NEWS_062;
using detail::fnews::WAR_NEWS_063;
using detail::fnews::WAR_NEWS_064;
using detail::fnews::WAR_NEWS_065;
using detail::fnews::WAR_NEWS_066;
using detail::fnews::WAR_NEWS_067;
using detail::fnews::WAR_NEWS_068;
using detail::fnews::WAR_NEWS_069;
using detail::fnews::WAR_NEWS_070;
using detail::fnews::WAR_NEWS_071;
using detail::fnews::WAR_NEWS_072;
using detail::fnews::WAR_NEWS_073;
using detail::fnews::WAR_NEWS_074;
using detail::fnews::WAR_NEWS_075;
using detail::fnews::WAR_NEWS_076;
using detail::fnews::WAR_NEWS_077;
using detail::fnews::WAR_NEWS_078;
using detail::fnews::WAR_NEWS_079;
using detail::fnews::WAR_NEWS_080;
using detail::fnews::WAR_NEWS_081;
using detail::fnews::WAR_NEWS_082;
using detail::fnews::WAR_NEWS_083;
using detail::fnews::WAR_NEWS_084;
using detail::fnews::WAR_NEWS_085;
using detail::fnews::WAR_NEWS_086;
using detail::fnews::WAR_NEWS_087;
using detail::fnews::WAR_NEWS_088;
using detail::fnews::WAR_NEWS_089;
using detail::fnews::WAR_NEWS_090;
using detail::fnews::WAR_NEWS_091;
using detail::fnews::WAR_NEWS_092;
using detail::fnews::WAR_NEWS_093;
using detail::fnews::WAR_NEWS_094;
using detail::fnews::WAR_NEWS_095;
using detail::fnews::WAR_NEWS_096;
using detail::fnews::WAR_NEWS_097;
using detail::fnews::WAR_NEWS_098;
using detail::fnews::WAR_NEWS_099;
using detail::fnews::WAR_NEWS_100;
using detail::fnews::WAR_NEWS_101;
using detail::fnews::WAR_NEWS_102;
using detail::fnews::WAR_NEWS_103;
using detail::fnews::WAR_NEWS_104;
using detail::fnews::WAR_NEWS_105;
using detail::fnews::WAR_NEWS_106;
using detail::fnews::WAR_NEWS_107;
using detail::fnews::WAR_NEWS_108;
using detail::fnews::WAR_NEWS_109;
using detail::fnews::WAR_NEWS_110;
using detail::fnews::WAR_NEWS_111;
using detail::fnews::WAR_NEWS_112;
using detail::fnews::WAR_NEWS_113;
using detail::fnews::WAR_NEWS_114;
using detail::fnews::WAR_NEWS_115;
using detail::fnews::WAR_NEWS_116;
using detail::fnews::WAR_NEWS_117;
using detail::fnews::WAR_NEWS_118;
using detail::fnews::WAR_NEWS_119;
using detail::fnews::WAR_NEWS_120;
using detail::fnews::WAR_NEWS_121;
using detail::fnews::WAR_NEWS_122;
using detail::fnews::WAR_NEWS_123;
using detail::fnews::WAR_NEWS_124;
using detail::fnews::WAR_NEWS_125;
using detail::fnews::WAR_NEWS_126;
using detail::fnews::WAR_NEWS_127;
using detail::fnews::WAR_NEWS_128;
using detail::fnews::WAR_NEWS_129;
using detail::fnews::WAR_NEWS_130;
using detail::fnews::WAR_NEWS_131;
using detail::fnews::WAR_NEWS_132;
using detail::fnews::WAR_NEWS_133;
using detail::fnews::WAR_NEWS_134;
using detail::fnews::WAR_NEWS_135;
using detail::fnews::WAR_NEWS_136;
using detail::fnews::WAR_NEWS_137;
using detail::fnews::WAR_NEWS_138;
using detail::fnews::WAR_NEWS_139;
using detail::fnews::WAR_NEWS_140;
using detail::fnews::WAR_NEWS_141;
} // namespace opeu::game
