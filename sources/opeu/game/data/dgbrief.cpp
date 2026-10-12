// Data segment DS:5bb2..7359 (6056 bytes). Scenario briefing and commander speech texts.
#include "data/dgbrief.hpp"
#include "data/image.hpp"

namespace opeu::game::detail::dgbrief {

auto InitialBriefingText(DataSegment& data) -> void {
  SetText(data.text_5bb2,
          "Destroy France! Split their front lines with a bilitzkreig attack. "
          "Kill the French bourgeosie.\033S");
  SetText(data.text_5c13,
          "Your mission is to occupy either San Michel or Rethel.\033S");
  SetText(data.text_5c4c,
          "But it's also imperative that you take Sedan as well.\033S");
  SetText(data.text_5c84, "You must seize these cities quickly.\033S");
  SetText(data.text_5cab,
          "They are the key to\012our victory over the\012French.\033S");
  SetText(
      data.text_5cde,
      "I'm counting on you, Commandant %s. Good luck.\012Heil Hitler!\033S");
  SetText(
      data.text_5d1c,
      "Just hours ago, the Germans crossed the Holland-Belgium border.\033S");
  SetText(data.text_5d5e, "You must prevent the Germans from taking France as "
                          "they did Poland!\033S");
  SetText(data.text_5da4,
          "You must stop the German advance by any means possible!\033S");
  SetText(data.text_5dde,
          "Now that the Germans have crossed the Maginot front,\033S");
  SetText(
      data.text_5e15,
      "we must protect Sedan. San Michel and Rethel are our last line\033S");
  SetText(data.text_5e56, "of defense. If these two cities are taken,\033S");
  SetText(data.text_5e83, "France will fall into German hands.\033S");
  SetText(data.text_5ea9,
          "All of France is counting on you, Commander %s.\033S");
  SetText(data.text_5edb, "If our Afrika Corps are victorious,\033S");
  SetText(data.text_5f01,
          "our control of the Mediterranean will be complete.\033S");
  SetText(data.text_5f36, "Your main goal is\012to occupy Tobruk.\033S");
  SetText(data.text_5f5c, "But if the enemy is too tenacious,\033S");
  SetText(data.text_5f81, "occupy Sidi Rezegh and Belhamed and cut off \033S");
  SetText(data.text_5fb0, "their supply routes. This should ensure \033S");
  SetText(data.text_5fdb, "your victory. I look forward to good news \033S");
  SetText(data.text_6008,
          "from the African Front, Commandant %s.\012Heil Hitler!\033S");
  SetText(data.text_603e,
          "Our defeat at Cyrenaica has not only hurt us on\033S");
  SetText(data.text_6070, "the African front, but threatens our control of the "
                          "Mediterranean.\033S");
  SetText(data.text_60b5, "We cannot afford to pull back any further.\033S");
  SetText(data.text_60e2,
          "Your orders are to defend Tobruk with your life.\033S");
  SetText(data.text_6115,
          "Your success depends on securing the supply lines\033S");
  SetText(data.text_6149,
          "to Belhamed and Sidi Rezegh. The enemy is worn out;\033S");
  SetText(data.text_617f,
          "if the battle is prolonged, victory will be ours.\033S");
  SetText(data.text_61b3, "Don't let us down, Commander %s.\033S");
  SetText(data.text_61d6, "Giving up Stalingrad does not mean our defeat on "
                          "the Eastern front.\033S");
  SetText(data.text_621c, "The Russians are merely putting up a desperate, "
                          "frenzied resistance.\033S");
  SetText(data.text_6263,
          "'Operation Barbarossa' will be reborn as 'Operation Citadel',\033S");
  SetText(data.text_62a3,
          "and the Russians will fall... in great numbers.\033S");
  SetText(data.text_62d5, "Commandant %s, your mission is to march \033S");
  SetText(data.text_6300, "from the south of Kursk and capture Belgorod.\033S");
  SetText(data.text_6330,
          "Then occupy Oboyan and Prokhorovka as quickly as possible.\033S");
  SetText(data.text_636d,
          "The Russians, with their supplies, will win if the battle is \033S");
  SetText(data.text_63ad,
          "prolonged. Blitzkreig your way to victory! Heil Hitler!\033S");
  SetText(data.text_63e7,
          "According to our comrades in Berlin, we know that the \033S");
  SetText(data.text_6420,
          "Germans are planning a pincer operation on Kursk.\033S");
  SetText(data.text_6454,
          "Fortunately, they don't know that we have this information.\033S");
  SetText(data.text_6492,
          "When the German troops launch their foolhardy offensive,\033S");
  SetText(data.text_64cd,
          "the Red Army will be ready... and we will destroy them!\033S");
  SetText(data.text_6507, "General %s,\012we need you to hold Belgorod, \033S");
  SetText(data.text_6534,
          "Prokhorovka, and Oboyan until reinforcements arrive.\033S");
  SetText(data.text_656b,
          "The reinforcements should arrive within a week.\033S");
  SetText(data.text_659d,
          "Bide your time, and then counter-attack with fresh men!\033S");
  SetText(data.text_65d7,
          "The steel hammer of justice will smash the imperialists!\033S");
  SetText(data.text_6612, "The Allies have landed at Normandy?!\033S");
  SetText(data.text_6639, "You must repel them at once!\033S");
  SetText(data.text_6658,
          "The 'Wall of the Atlantic' will be your support. \033S");
  SetText(data.text_668c,
          "Crush them at the shore, and show them the power of Germany!\033S");
  SetText(data.text_66cb,
          "Using Caen, Bayeux and St. Lo as your defensive footholds,\033S");
  SetText(
      data.text_6708,
      "take the coastal cities of Vierville, Arromanches, Courseulles,\033S");
  SetText(data.text_674a,
          "and Lion-sur-Mer. Then cut off their supply\012routes.\033S");
  SetText(data.text_6780,
          "This is your mission, Commandant %s. Do not fail us.\033S");
  SetText(data.text_67b7,
          "A quick occupation will be the key to victory.\033S");
  SetText(
      data.text_67e8,
      "The Allies must not reach our homeland of Germany! Heil Hitler!\033S");
  SetText(
      data.text_682a,
      "At the Tehran conference last year, US-British forces promised\033S");
  SetText(data.text_686b, "the Russians that we would make a second battle "
                          "front in Western Europe.\033S");
  SetText(data.text_68b6, "This is our chance!!\033S");
  SetText(data.text_68cd,
          "Now is the time to launch 'Operation Overlord'!\033S");
  SetText(data.text_68ff,
          "After landing at Normandy, I would like you and your men\033S");
  SetText(data.text_693a, "to proceed inland into France and capture as many "
                          "cities as\012possible.\033S");
  SetText(data.text_6982, "You must take the cities Caen, Bayeux and St. Lo, "
                          "as well as secure the beachfronts at\033S");
  SetText(data.text_69db,
          "Vierville, Arromanches, Courseulles, and Lion-sur-Mer.\033S");
  SetText(data.text_6a14, "The French people are counting on you to liberate "
                          "them, Commander %s.\033S");
  SetText(data.text_6a5c,
          "The US-British forces are too anxious for victory.\033S");
  SetText(
      data.text_6a91,
      "They thought that Germany would fall quickly. They were mistaken.\033S");
  SetText(data.text_6ad5, "Now their front line is overextended and their "
                          "soldiers exhausted.\033S");
  SetText(data.text_6b1a,
          "This time has come to launch 'Operation Autumn Mist!'\033S");
  SetText(data.text_6b52, "Remember that the fate of the empire rests on the "
                          "outcome of this battle!!\033S");
  SetText(data.text_6b9f, "Our main goal is to\012break through the "
                          "weak\012enemy lines at\012Ardennes,\033S");
  SetText(data.text_6be5,
          "retake Antwerp, and cut off enemy supply lines.\033S");
  SetText(data.text_6c17,
          "We need you to capture Bastogne. If we take that city,\033S");
  SetText(data.text_6c50, "it will be easier for the armored divisions headed "
                          "for the Meuse river to offer support.\033S");
  SetText(data.text_6cab,
          "I await your victory report, Commandant %s. Heil Hitler!\033S");
  SetText(data.text_6ce6, "I'm surprised that Germany still has enough "
                          "strength to counter-attack,\033S");
  SetText(data.text_6d30, "but there's no cause for alarm.\033S");
  SetText(data.text_6d52,
          "We will not see a repeat of the 1940 German-French battle.\033S");
  SetText(data.text_6d8f, "We are going to tear them to shreds!\033S");
  SetText(data.text_6db6,
          "I will be sending reinforcements from the Saar region.\033S");
  SetText(
      data.text_6def,
      "We need you to defend Bastogne until the reinforcements arrive.\033S");
  SetText(data.text_6e31, "If Bastogne is taken, it will open a large hole in "
                          "our front line;\033S");
  SetText(data.text_6e76,
          "we may even lose any chance to counter-attack.\033S");
  SetText(data.text_6ea7, "Let's show those German pansies who's boss!\033S");
  SetText(data.text_6ed5, "I will not desert Berlin.\033S");
  SetText(data.text_6ef1, "Neither the all-out torching of our Germany\033S");
  SetText(
      data.text_6f1f,
      "nor the total destruction of our armies will sway my decision.\033S");
  SetText(data.text_6f60,
          "We will continue to fight until the last German falls!\033S");
  SetText(data.text_6f99, "Our military empire cannot be stopped!\033S");
  SetText(data.text_6fc2, "Roosevelt's death will inevitably divide Europe "
                          "into East and West.\033S");
  SetText(data.text_7008,
          "If we stop enemy troops at Frankfurt and Strausberg,\033S");
  SetText(data.text_703f,
          "we may buy enough time to take advantage of the Allied split\033S");
  SetText(data.text_707e, "and revive our great empire.\033S");
  SetText(data.text_709d,
          "Commander %s, the future of Germany rests in your hands.\033S");
  SetText(data.text_70d8,
          "Hail to the German Empire in all its glory! Heil Hitler!\033S");
  SetText(data.text_7113, "Soviet comrades, victory is close at hand!\033S");
  SetText(data.text_7140, "The time for us to take revenge has come!\033S");
  SetText(data.text_716c,
          "Remember Hitler! Remember the Nazi atrocities!\033S");
  SetText(data.text_719d, "Your friends, your families, all killed at the "
                          "hands of SS Troops!\033S");
  SetText(data.text_71e2,
          "Hear me, Soviet comrades! It is time for revenge!!\033S");
  SetText(data.text_7217, "Your mission, Commander %s,\033S");
  SetText(data.text_7235,
          "is to cross the Oder river and open a route to Berlin.\033S");
  SetText(data.text_726e, "Take Frankfurt and Strausberg\033S");
  SetText(data.text_728e, "and rain destruction down upon the Germans!\033S");
  SetText(data.text_72bc, "It will not be the Americans or the British who "
                          "finish off Germany, my friends.\033S");
  SetText(data.text_730e, "We Soviets will deal the final blow. Remember that "
                          "when you storm Berlin!\033S");
}
} // namespace opeu::game::detail::dgbrief
