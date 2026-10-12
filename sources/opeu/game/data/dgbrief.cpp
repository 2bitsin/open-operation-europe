// Data segment DS:5bb2..7359 (6056 bytes). Scenario briefing and commander speech texts.
#include "data/dgbrief.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::dgbrief {

auto InitialBriefingText(DataSegment& data) -> void {
  SetText(data.brief_france_axis_1, "Destroy France! Split their front lines with a bilitzkreig attack. "
                                    "Kill the French bourgeosie.\033S");
  SetText(data.brief_france_axis_2, "Your mission is to occupy either San Michel or Rethel.\033S");
  SetText(data.brief_france_axis_3, "But it's also imperative that you take Sedan as well.\033S");
  SetText(data.brief_france_axis_4, "You must seize these cities quickly.\033S");
  SetText(data.brief_france_axis_5, "They are the key to\012our victory over the\012French.\033S");
  SetText(data.brief_france_axis_6, "I'm counting on you, Commandant %s. Good luck.\012Heil Hitler!\033S");
  SetText(data.brief_france_allies_1, "Just hours ago, the Germans crossed the Holland-Belgium border.\033S");
  SetText(data.brief_france_allies_2, "You must prevent the Germans from taking France as "
                                      "they did Poland!\033S");
  SetText(data.brief_france_allies_3, "You must stop the German advance by any means possible!\033S");
  SetText(data.brief_france_allies_4, "Now that the Germans have crossed the Maginot front,\033S");
  SetText(data.brief_france_allies_5, "we must protect Sedan. San Michel and Rethel are our last line\033S");
  SetText(data.brief_france_allies_6, "of defense. If these two cities are taken,\033S");
  SetText(data.brief_france_allies_7, "France will fall into German hands.\033S");
  SetText(data.brief_france_allies_8, "All of France is counting on you, Commander %s.\033S");
  SetText(data.brief_africa_axis_1, "If our Afrika Corps are victorious,\033S");
  SetText(data.brief_africa_axis_2, "our control of the Mediterranean will be complete.\033S");
  SetText(data.brief_africa_axis_3, "Your main goal is\012to occupy Tobruk.\033S");
  SetText(data.brief_africa_axis_4, "But if the enemy is too tenacious,\033S");
  SetText(data.brief_africa_axis_5, "occupy Sidi Rezegh and Belhamed and cut off \033S");
  SetText(data.brief_africa_axis_6, "their supply routes. This should ensure \033S");
  SetText(data.brief_africa_axis_7, "your victory. I look forward to good news \033S");
  SetText(data.brief_africa_axis_8, "from the African Front, Commandant %s.\012Heil Hitler!\033S");
  SetText(data.brief_africa_allies_1, "Our defeat at Cyrenaica has not only hurt us on\033S");
  SetText(data.brief_africa_allies_2, "the African front, but threatens our control of the "
                                      "Mediterranean.\033S");
  SetText(data.brief_africa_allies_3, "We cannot afford to pull back any further.\033S");
  SetText(data.brief_africa_allies_4, "Your orders are to defend Tobruk with your life.\033S");
  SetText(data.brief_africa_allies_5, "Your success depends on securing the supply lines\033S");
  SetText(data.brief_africa_allies_6, "to Belhamed and Sidi Rezegh. The enemy is worn out;\033S");
  SetText(data.brief_africa_allies_7, "if the battle is prolonged, victory will be ours.\033S");
  SetText(data.brief_africa_allies_8, "Don't let us down, Commander %s.\033S");
  SetText(data.brief_kursk_axis_1, "Giving up Stalingrad does not mean our defeat on "
                                   "the Eastern front.\033S");
  SetText(data.brief_kursk_axis_2, "The Russians are merely putting up a desperate, "
                                   "frenzied resistance.\033S");
  SetText(data.brief_kursk_axis_3, "'Operation Barbarossa' will be reborn as 'Operation Citadel',\033S");
  SetText(data.brief_kursk_axis_4, "and the Russians will fall... in great numbers.\033S");
  SetText(data.brief_kursk_axis_5, "Commandant %s, your mission is to march \033S");
  SetText(data.brief_kursk_axis_6, "from the south of Kursk and capture Belgorod.\033S");
  SetText(data.brief_kursk_axis_7, "Then occupy Oboyan and Prokhorovka as quickly as possible.\033S");
  SetText(data.brief_kursk_axis_8, "The Russians, with their supplies, will win if the battle is \033S");
  SetText(data.brief_kursk_axis_9, "prolonged. Blitzkreig your way to victory! Heil Hitler!\033S");
  SetText(data.brief_kursk_allies_1, "According to our comrades in Berlin, we know that the \033S");
  SetText(data.brief_kursk_allies_2, "Germans are planning a pincer operation on Kursk.\033S");
  SetText(data.brief_kursk_allies_3, "Fortunately, they don't know that we have this information.\033S");
  SetText(data.brief_kursk_allies_4, "When the German troops launch their foolhardy offensive,\033S");
  SetText(data.brief_kursk_allies_5, "the Red Army will be ready... and we will destroy them!\033S");
  SetText(data.brief_kursk_allies_6, "General %s,\012we need you to hold Belgorod, \033S");
  SetText(data.brief_kursk_allies_7, "Prokhorovka, and Oboyan until reinforcements arrive.\033S");
  SetText(data.brief_kursk_allies_8, "The reinforcements should arrive within a week.\033S");
  SetText(data.brief_kursk_allies_9, "Bide your time, and then counter-attack with fresh men!\033S");
  SetText(data.brief_kursk_allies_10, "The steel hammer of justice will smash the imperialists!\033S");
  SetText(data.brief_normandy_axis_1, "The Allies have landed at Normandy?!\033S");
  SetText(data.brief_normandy_axis_2, "You must repel them at once!\033S");
  SetText(data.brief_normandy_axis_3, "The 'Wall of the Atlantic' will be your support. \033S");
  SetText(data.brief_normandy_axis_4, "Crush them at the shore, and show them the power of Germany!\033S");
  SetText(data.brief_normandy_axis_5, "Using Caen, Bayeux and St. Lo as your defensive footholds,\033S");
  SetText(data.brief_normandy_axis_6, "take the coastal cities of Vierville, Arromanches, Courseulles,\033S");
  SetText(data.brief_normandy_axis_7, "and Lion-sur-Mer. Then cut off their supply\012routes.\033S");
  SetText(data.brief_normandy_axis_8, "This is your mission, Commandant %s. Do not fail us.\033S");
  SetText(data.brief_normandy_axis_9, "A quick occupation will be the key to victory.\033S");
  SetText(data.brief_normandy_axis_10, "The Allies must not reach our homeland of Germany! Heil Hitler!\033S");
  SetText(data.brief_normandy_allies_1, "At the Tehran conference last year, US-British forces promised\033S");
  SetText(data.brief_normandy_allies_2, "the Russians that we would make a second battle "
                                        "front in Western Europe.\033S");
  SetText(data.brief_normandy_allies_3, "This is our chance!!\033S");
  SetText(data.brief_normandy_allies_4, "Now is the time to launch 'Operation Overlord'!\033S");
  SetText(data.brief_normandy_allies_5, "After landing at Normandy, I would like you and your men\033S");
  SetText(data.brief_normandy_allies_6, "to proceed inland into France and capture as many "
                                        "cities as\012possible.\033S");
  SetText(data.brief_normandy_allies_7, "You must take the cities Caen, Bayeux and St. Lo, "
                                        "as well as secure the beachfronts at\033S");
  SetText(data.brief_normandy_allies_8, "Vierville, Arromanches, Courseulles, and Lion-sur-Mer.\033S");
  SetText(data.brief_normandy_allies_9, "The French people are counting on you to liberate "
                                        "them, Commander %s.\033S");
  SetText(data.brief_ardennes_axis_1, "The US-British forces are too anxious for victory.\033S");
  SetText(data.brief_ardennes_axis_2, "They thought that Germany would fall quickly. They were mistaken.\033S");
  SetText(data.brief_ardennes_axis_3, "Now their front line is overextended and their "
                                      "soldiers exhausted.\033S");
  SetText(data.brief_ardennes_axis_4, "This time has come to launch 'Operation Autumn Mist!'\033S");
  SetText(data.brief_ardennes_axis_5, "Remember that the fate of the empire rests on the "
                                      "outcome of this battle!!\033S");
  SetText(data.brief_ardennes_axis_6, "Our main goal is to\012break through the "
                                      "weak\012enemy lines at\012Ardennes,\033S");
  SetText(data.brief_ardennes_axis_7, "retake Antwerp, and cut off enemy supply lines.\033S");
  SetText(data.brief_ardennes_axis_8, "We need you to capture Bastogne. If we take that city,\033S");
  SetText(data.brief_ardennes_axis_9, "it will be easier for the armored divisions headed "
                                      "for the Meuse river to offer support.\033S");
  SetText(data.brief_ardennes_axis_10, "I await your victory report, Commandant %s. Heil Hitler!\033S");
  SetText(data.brief_ardennes_allies_1, "I'm surprised that Germany still has enough "
                                        "strength to counter-attack,\033S");
  SetText(data.brief_ardennes_allies_2, "but there's no cause for alarm.\033S");
  SetText(data.brief_ardennes_allies_3, "We will not see a repeat of the 1940 German-French battle.\033S");
  SetText(data.brief_ardennes_allies_4, "We are going to tear them to shreds!\033S");
  SetText(data.brief_ardennes_allies_5, "I will be sending reinforcements from the Saar region.\033S");
  SetText(data.brief_ardennes_allies_6, "We need you to defend Bastogne until the reinforcements arrive.\033S");
  SetText(data.brief_ardennes_allies_7, "If Bastogne is taken, it will open a large hole in "
                                        "our front line;\033S");
  SetText(data.brief_ardennes_allies_8, "we may even lose any chance to counter-attack.\033S");
  SetText(data.brief_ardennes_allies_9, "Let's show those German pansies who's boss!\033S");
  SetText(data.brief_berlin_axis_1, "I will not desert Berlin.\033S");
  SetText(data.brief_berlin_axis_2, "Neither the all-out torching of our Germany\033S");
  SetText(data.brief_berlin_axis_3, "nor the total destruction of our armies will sway my decision.\033S");
  SetText(data.brief_berlin_axis_4, "We will continue to fight until the last German falls!\033S");
  SetText(data.brief_berlin_axis_5, "Our military empire cannot be stopped!\033S");
  SetText(data.brief_berlin_axis_6, "Roosevelt's death will inevitably divide Europe "
                                    "into East and West.\033S");
  SetText(data.brief_berlin_axis_7, "If we stop enemy troops at Frankfurt and Strausberg,\033S");
  SetText(data.brief_berlin_axis_8, "we may buy enough time to take advantage of the Allied split\033S");
  SetText(data.brief_berlin_axis_9, "and revive our great empire.\033S");
  SetText(data.brief_berlin_axis_10, "Commander %s, the future of Germany rests in your hands.\033S");
  SetText(data.brief_berlin_axis_11, "Hail to the German Empire in all its glory! Heil Hitler!\033S");
  SetText(data.brief_berlin_allies_1, "Soviet comrades, victory is close at hand!\033S");
  SetText(data.brief_berlin_allies_2, "The time for us to take revenge has come!\033S");
  SetText(data.brief_berlin_allies_3, "Remember Hitler! Remember the Nazi atrocities!\033S");
  SetText(data.brief_berlin_allies_4, "Your friends, your families, all killed at the "
                                      "hands of SS Troops!\033S");
  SetText(data.brief_berlin_allies_5, "Hear me, Soviet comrades! It is time for revenge!!\033S");
  SetText(data.brief_berlin_allies_6, "Your mission, Commander %s,\033S");
  SetText(data.brief_berlin_allies_7, "is to cross the Oder river and open a route to Berlin.\033S");
  SetText(data.brief_berlin_allies_8, "Take Frankfurt and Strausberg\033S");
  SetText(data.brief_berlin_allies_9, "and rain destruction down upon the Germans!\033S");
  SetText(data.brief_berlin_allies_10, "It will not be the Americans or the British who "
                                       "finish off Germany, my friends.\033S");
  SetText(data.brief_berlin_allies_11, "We Soviets will deal the final blow. Remember that "
                                       "when you storm Berlin!\033S");
}
} // namespace opeu::game::detail::dgbrief
