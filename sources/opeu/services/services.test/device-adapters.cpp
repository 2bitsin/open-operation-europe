// The native files, input, sound and clock forward each method to the service function the game called: what one
// does, the other sees.
#include "native-machine.hpp"

#include "hostinput.hpp"
#include "platform.hpp"
#include "port.hpp"
#include "services.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace opeu::services {
namespace {

// Headless: the pump advances virtual time and touches no window.
class Headless : public testing::Test {
protected:
  auto SetUp() -> void override {
    _was_headless         = port_options.headless;
    port_options.headless = 1;
  }
  auto TearDown() -> void override {
    port_options.headless = _was_headless;
  }

private:
  int _was_headless{};
};

class FilesAdapter : public testing::Test {
protected:
  auto SetUp() -> void override {
    _saves = std::filesystem::temp_directory_path() / ("opeu-files-adapter-" + std::string{ testing::UnitTest::GetInstance()->current_test_info()->name() });
    std::filesystem::remove_all(_saves);
    plat_set_save_dir(_saves.string());
  }
  auto TearDown() -> void override {
    std::filesystem::remove_all(_saves);
  }

  NativeFiles           _files;
  std::filesystem::path _saves;
};

TEST_F(FilesAdapter, WritesWhatTheDosLayerReads) {
  constexpr std::array<std::uint8_t, 5> BYTES{ 'S', 'A', 'V', 'E', 'D' };
  auto const                            created{ _files.Create("GAME1.DAT") };
  ASSERT_TRUE(created.has_value());
  EXPECT_EQ(_files.Write(*created, BYTES), BYTES.size());
  _files.Close(*created);
  EXPECT_NE(opeu_dos_exists("GAME1.DAT"), 0);
  auto const                  handle{ opeu_dos_open("GAME1.DAT", 0) };
  std::array<std::uint8_t, 5> read{};
  EXPECT_EQ(opeu_dos_read(handle, read.data(), read.size()), 5);
  opeu_dos_close(handle);
  EXPECT_EQ(read, BYTES);
}

TEST_F(FilesAdapter, ReadsAndSeeksWhatTheDosLayerWrote) {
  constexpr std::array<std::uint8_t, 6> BYTES{ 1, 2, 3, 4, 5, 6 };
  auto const                            handle{ opeu_dos_create("GAME2.DAT") };
  opeu_dos_write(handle, BYTES.data(), BYTES.size());
  opeu_dos_close(handle);
  EXPECT_TRUE(_files.Exists("GAME2.DAT"));
  auto const opened{ _files.Open("GAME2.DAT", OpenMode::READ_WRITE) };
  ASSERT_TRUE(opened.has_value());
  EXPECT_EQ(_files.Seek(*opened, 2, SeekOrigin::START), 2);
  std::array<std::uint8_t, 3> read{};
  EXPECT_EQ(_files.Read(*opened, read), 3);
  EXPECT_EQ(read, (std::array<std::uint8_t, 3>{ 3, 4, 5 }));
  EXPECT_EQ(_files.Seek(*opened, -1, SeekOrigin::END), 5);
  _files.ResetDisk();
  _files.Close(*opened);
}

TEST_F(FilesAdapter, AnswersNoneWhereTheDosLayerFails) {
  EXPECT_FALSE(_files.Open("ABSENT.DAT", OpenMode::READ).has_value());
  EXPECT_FALSE(_files.Exists("ABSENT.DAT"));
}

class InputAdapter : public Headless {
protected:
  NativeInput _input;
};

TEST_F(InputAdapter, ReadsTheKeysTheKeyboardHolds) {
  KeyPressed(0x1C, '\r');
  EXPECT_TRUE(_input.KeyWaiting());
  EXPECT_EQ(_input.ReadKey(), 0x1C0D);
  EXPECT_FALSE(_input.KeyWaiting());
  KeyPressed(0x3B, 0); // F1: no ASCII, the scan code next
  EXPECT_EQ(_input.ReadCharacter(), 0);
  EXPECT_EQ(_input.ReadCharacter(), 0x3B);
}

TEST_F(InputAdapter, ReportsTheMouseTheDriverHolds) {
  EXPECT_TRUE(_input.ResetMouse());
  _input.LimitPointerX({ 0, 639 });
  _input.LimitPointerY({ 40, 439 });
  _input.MovePointer({ 123, 200 });
  _input.ShowPointer();
  std::uint16_t buttons{};
  std::uint16_t x{};
  std::uint16_t y{};
  port_mouse_state(&buttons, &x, &y);
  EXPECT_EQ(_input.Mouse(), (MouseState{ .buttons = buttons, .x = x, .y = y }));
  EXPECT_EQ(_input.Mouse().x, 123);
  _input.HidePointer();
}

TEST_F(InputAdapter, PumpsVirtualTime) {
  auto const before{ port_time_us() };
  _input.Pump();
  EXPECT_EQ(port_time_us(), before + 1000);
}

class SoundAdapter : public Headless {
protected:
  NativeSound _sound;
};

TEST_F(SoundAdapter, AsksTheDriverWhatTheGameAsked) {
  EXPECT_EQ(_sound.MusicPlaying(), (port_fm_function(0x03, 0, 0, 0, PORT_DS_SEGMENT) & 0xFF) != 0);
  EXPECT_EQ(_sound.EffectPlaying(), (port_fm_function(0x13, 0, 0, 0, PORT_DS_SEGMENT) & 0xFF) != 0);
  _sound.StartCountdown(7);
  EXPECT_EQ(_sound.CountdownLeft(), opeu_fm_busy());
}

class ClockAdapter : public Headless {
protected:
  NativeClock _clock;
};

TEST_F(ClockAdapter, ReadsTheBiosTicks) {
  EXPECT_EQ(_clock.Ticks(), port_clock_ticks_low());
}

}
}
