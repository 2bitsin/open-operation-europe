// The native machine: one adapter per device interface, each method forwarding to the services' function the game
// called before the interfaces existed (the mapping is the table in docs/platform-surface.md). Stateless: the state is
// the services' own (the screen, the file table, the keyboard and mouse, the FM driver).
#pragma once

#include "machine.hpp"

namespace opeu::services::detail::native_machine {

class NativeDisplay final : public services::Display {
public:
  auto EnterGraphics() -> void override;
  auto LeaveGraphics() -> void override;
  auto Clear() -> void override;
  auto ShowIdentityPalette() -> void override;
  auto LoadPalette(std::uint8_t first, std::span<std::uint8_t const> levels) -> void override;
  auto DrawColumns(Area area, Image image) -> bool override;
  auto DrawThreeByteColumns(Area area, Image image) -> bool override;
  auto DrawBits(Area area, Image image) -> bool override;
  auto DrawBitsOnPaper(Area area, Image image) -> void override;
  auto DrawShiftedBits(Area area, Image image) -> bool override;
  auto DrawShiftedColumns(Area area, std::span<std::uint8_t const> image) -> void override;
  auto SaveColumns(Area area, std::span<std::uint8_t> into) -> void override;
  auto CopyColumns(Area from, Point to) -> void override;
  auto DecodePicture(Point at, PictureByteReader const& reader) -> bool override;
  auto DecodeMaskedPicture(Point at, PictureByteReader const& reader) -> bool override;
  auto Box(Area area, std::uint16_t colour) -> bool override;
  auto Highlight(Area area, std::uint16_t colour) -> bool override;
  auto Line(Corners ends, std::uint16_t colour) -> void override;
  auto Fill(Corners corners, std::uint16_t colour) -> void override;
  auto SaveCell(Point at, Cell into) -> void override;
  auto SaveShiftedCell(Point at, Cell into) -> void override;
  auto RestoreCell(Point at, CellImage cell) -> void override;
  auto RestoreShiftedCell(Point at, CellImage cell) -> void override;
  auto CopyCellBlock(Point from, Point to) -> void override;
  auto OverlayCell(Cell cell, CellImage image) -> void override;
  auto MaskCell(Cell cell, CellMask mask) -> void override;
  auto DrawCellGlyph(Cell cell, CellGlyph glyph, Point at) -> void override;
  auto DrawColouredCellGlyph(Cell cell, CellGlyph glyph, ColouredPlace place) -> void override;
  auto ComposeSprite(Area area, Sprite const& sprite) -> void override;
  auto ComposeShiftedSprite(Area area, Sprite const& sprite) -> void override;
};

class NativeFiles final : public services::Files {
public:
  auto Open(std::string_view name, OpenMode mode) -> std::optional<FileHandle> override;
  auto Create(std::string_view name) -> std::optional<FileHandle> override;
  auto Exists(std::string_view name) -> bool override;
  auto Close(FileHandle file) -> void override;
  auto Read(FileHandle file, std::span<std::uint8_t> into) -> std::optional<std::uint16_t> override;
  auto ReadFar(FileHandle file, FarRegion into) -> std::optional<std::uint16_t> override;
  auto Write(FileHandle file, std::span<std::uint8_t const> bytes) -> std::optional<std::uint16_t> override;
  auto Seek(FileHandle file, std::int32_t position, SeekOrigin origin) -> std::optional<std::int32_t> override;
  auto ResetDisk() -> void override;
};

class NativeInput final : public services::Input {
public:
  auto KeyWaiting() -> bool override;
  auto ReadKey() -> std::uint16_t override;
  auto ReadCharacter() -> std::uint8_t override;
  auto ResetMouse() -> bool override;
  auto ShowPointer() -> void override;
  auto HidePointer() -> void override;
  auto Mouse() -> MouseState override;
  auto MovePointer(Point to) -> void override;
  auto LimitPointerX(Range columns) -> void override;
  auto LimitPointerY(Range lines) -> void override;
  auto Pump() -> void override;
};

class NativeSound final : public services::Sound {
public:
  auto Open(FarAddress music, FarAddress effects) -> bool override;
  auto PlaySong(std::uint8_t song) -> void override;
  auto StopMusic(std::uint8_t fade) -> void override;
  auto MusicPlaying() -> bool override;
  auto SetRepeatMode(std::uint8_t mode) -> void override;
  auto SetVoiceInstrument(std::uint8_t voice, FarAddress instrument) -> void override;
  auto PlayVoiceNote(std::uint8_t voice, std::uint16_t note) -> void override;
  auto LoadEffects(FarAddress effects) -> void override;
  auto PlayEffect(std::uint8_t effect) -> void override;
  auto StopEffect() -> void override;
  auto KeyOffEffectVoice(std::uint8_t voice) -> void override;
  auto EffectPlaying() -> bool override;
  auto StartCountdown(std::uint16_t ticks) -> void override;
  auto CountdownLeft() -> std::uint16_t override;
};

class NativeClock final : public services::Clock {
public:
  auto Ticks() -> std::uint16_t override;
};

}

namespace opeu::services::detail::machine {

// The native machine's adapters: owned by a default-constructed Machine, or by the application entry that wires them.
class NativeDevices {
public:
  [[nodiscard]] auto Devices() noexcept -> machine::Devices {
    return { .display = _display, .files = _files, .input = _input, .sound = _sound, .clock = _clock };
  }

private:
  native_machine::NativeDisplay _display;
  native_machine::NativeFiles   _files;
  native_machine::NativeInput   _input;
  native_machine::NativeSound   _sound;
  native_machine::NativeClock   _clock;
};

}

namespace opeu::services {
using detail::machine::NativeDevices;
using detail::native_machine::NativeClock;
using detail::native_machine::NativeDisplay;
using detail::native_machine::NativeFiles;
using detail::native_machine::NativeInput;
using detail::native_machine::NativeSound;
}
