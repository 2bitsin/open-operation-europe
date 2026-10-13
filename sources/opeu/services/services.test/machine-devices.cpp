// A machine is the native one unless a test hands it devices; the fakes behave as the game expects of the devices.
#include "native-machine.hpp"

#include "fake-machine.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>

namespace opeu::services {
namespace {

TEST(Machine, IsNativeByDefault) {
  Machine machine;
  EXPECT_NE(dynamic_cast<NativeDisplay*>(&machine.Display()), nullptr);
  EXPECT_NE(dynamic_cast<NativeFiles*>(&machine.Files()), nullptr);
  EXPECT_NE(dynamic_cast<NativeInput*>(&machine.Input()), nullptr);
  EXPECT_NE(dynamic_cast<NativeSound*>(&machine.Sound()), nullptr);
  EXPECT_NE(dynamic_cast<NativeClock*>(&machine.Clock()), nullptr);
}

TEST(Machine, HandsOutTheDevicesItIsGiven) {
  fakes::FakeMachine fake;
  auto&              machine{ fake.Machine() };
  EXPECT_EQ(&machine.Display(), &fake.Display());
  EXPECT_EQ(&machine.Files(), &fake.Files());
  EXPECT_EQ(&machine.Input(), &fake.Input());
  EXPECT_EQ(&machine.Sound(), &fake.Sound());
  EXPECT_EQ(&machine.Clock(), &fake.Clock());
}

TEST(FakeDisplay, RecordsWhatItIsAskedToDraw) {
  fakes::FakeDisplay display;
  EXPECT_TRUE(display.Box({ 1, 2, 3, 4 }, 11));
  display.AnswerDrawn(false);
  EXPECT_FALSE(display.DrawColumns({ 5, 6, 7, 8 }, { .mode = 4, .bytes = {} }));
  EXPECT_EQ(display.Calls(), (std::vector<std::string>{ "Box 1 2 3 4 11", "DrawColumns 5 6 7 8 4 0" }));
}

TEST(FakeFiles, KeepsWhatIsWrittenForTheNextRead) {
  constexpr std::array<std::uint8_t, 4> BYTES{ 9, 8, 7, 6 };
  fakes::FakeFiles                      files;
  auto const                            created{ files.Create("SAVE.DAT") };
  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(files.Write(*created, BYTES), 4);
  EXPECT_EQ(files.Seek(*created, 1, SeekOrigin::START), 1);
  std::array<std::uint8_t, 4> read{};
  EXPECT_EQ(files.Read(*created, read), 3);
  EXPECT_EQ(read, (std::array<std::uint8_t, 4>{ 8, 7, 6, 0 }));
  files.Close(*created);
  EXPECT_TRUE(files.Exists("SAVE.DAT"));
  EXPECT_FALSE(files.Open("OTHER.DAT", OpenMode::READ).has_value());
}

TEST(FakeInput, PlaysBackTheKeysPressed) {
  fakes::FakeInput input;
  input.Press(0x1C0D);
  input.Press(0x3B00);
  EXPECT_TRUE(input.KeyWaiting());
  EXPECT_EQ(input.ReadKey(), 0x1C0D);
  EXPECT_EQ(input.ReadCharacter(), 0);
  EXPECT_EQ(input.ReadCharacter(), 0x3B);
  EXPECT_FALSE(input.KeyWaiting());
  input.MovePointer({ 10, 50 });
  EXPECT_EQ(input.Mouse(), (MouseState{ .buttons = 0, .x = 10, .y = 50 }));
}

TEST(FakeSound, PlaysUntilStopped) {
  fakes::FakeSound sound;
  sound.PlaySong(3);
  EXPECT_TRUE(sound.MusicPlaying());
  sound.StopMusic(20);
  EXPECT_FALSE(sound.MusicPlaying());
  EXPECT_EQ(sound.Calls(), (std::vector<std::string>{ "PlaySong 3", "StopMusic 20" }));
}

TEST(FakeClock, ReadsTheTicksSet) {
  fakes::FakeClock clock;
  clock.Set(0x1234);
  EXPECT_EQ(clock.Ticks(), 0x1234);
}

}
}
