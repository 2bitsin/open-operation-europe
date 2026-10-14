// Fakes of the machine's devices for tests: the display and the sound record what they are asked, the files keep
// their contents in memory, the input plays back queued keys and a set mouse, the clock reads a set tick count.
// FakeMachine bundles them into a Machine a test hands the game.
#pragma once

#include "machine.hpp"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opeu::services::fakes {

// What a fake was asked, one line per call ("Box 1 2 3 4 11").
class CallLog {
public:
  template <typename... _Arguments>
  auto Record(std::string_view const call, _Arguments const&... arguments) -> void {
    auto line{ std::string{ call } };
    ((line += std::format(" {}", arguments)), ...);
    _calls.push_back(std::move(line));
  }
  [[nodiscard]] auto Calls() const -> std::vector<std::string> const& {
    return _calls;
  }

private:
  std::vector<std::string> _calls;
};

class FakeDisplay final : public Display, public CallLog {
public:
  // What the drawing methods that report it answer.
  auto AnswerDrawn(bool const drawn) -> void {
    _drawn = drawn;
  }

  auto EnterGraphics() -> void override {
    Record("EnterGraphics");
  }
  auto LeaveGraphics() -> void override {
    Record("LeaveGraphics");
  }
  auto Clear() -> void override {
    Record("Clear");
  }
  auto ShowIdentityPalette() -> void override {
    Record("ShowIdentityPalette");
  }
  auto LoadPalette(std::uint8_t const first, std::span<std::uint8_t const> const levels) -> void override {
    Record("LoadPalette", first, levels.size());
  }
  auto DrawColumns(Area const area, Image const image) -> bool override {
    return Drawing("DrawColumns", area, image);
  }
  auto DrawThreeByteColumns(Area const area, Image const image) -> bool override {
    return Drawing("DrawThreeByteColumns", area, image);
  }
  auto DrawBits(Area const area, Image const image) -> bool override {
    return Drawing("DrawBits", area, image);
  }
  auto DrawBitsOnPaper(Area const area, Image const image) -> void override {
    std::ignore = Drawing("DrawBitsOnPaper", area, image);
  }
  auto DrawShiftedBits(Area const area, Image const image) -> bool override {
    return Drawing("DrawShiftedBits", area, image);
  }
  auto DrawShiftedColumns(Area const area, std::span<std::uint8_t const> const image) -> void override {
    std::ignore = Drawing("DrawShiftedColumns", area, { .mode = 0, .bytes = image });
  }
  auto SaveColumns(Area const area, std::span<std::uint8_t> const into) -> void override {
    Record("SaveColumns", area.x, area.y, area.width, area.height, into.size());
  }
  auto CopyColumns(Area const from, Point const to) -> void override {
    Record("CopyColumns", from.x, from.y, from.width, from.height, to.x, to.y);
  }
  auto DecodePicture(Point const at, [[maybe_unused]] PictureByteReader const& reader) -> bool override {
    return Answer("DecodePicture", at.x, at.y);
  }
  auto DecodeMaskedPicture(Point const at, [[maybe_unused]] PictureByteReader const& reader) -> bool override {
    return Answer("DecodeMaskedPicture", at.x, at.y);
  }
  auto Box(Area const area, std::uint16_t const colour) -> bool override {
    return Answer("Box", area.x, area.y, area.width, area.height, colour);
  }
  auto Highlight(Area const area, std::uint16_t const colour) -> bool override {
    return Answer("Highlight", area.x, area.y, area.width, area.height, colour);
  }
  auto Line(Corners const ends, std::uint16_t const colour) -> void override {
    Record("Line", ends.from.x, ends.from.y, ends.to.x, ends.to.y, colour);
  }
  auto Fill(Corners const corners, std::uint16_t const colour) -> void override {
    Record("Fill", corners.from.x, corners.from.y, corners.to.x, corners.to.y, colour);
  }
  auto SaveCell(Point const at, [[maybe_unused]] Cell into) -> void override {
    Record("SaveCell", at.x, at.y);
  }
  auto SaveShiftedCell(Point const at, [[maybe_unused]] Cell into) -> void override {
    Record("SaveShiftedCell", at.x, at.y);
  }
  auto RestoreCell(Point const at, [[maybe_unused]] CellImage cell) -> void override {
    Record("RestoreCell", at.x, at.y);
  }
  auto RestoreShiftedCell(Point const at, [[maybe_unused]] CellImage cell) -> void override {
    Record("RestoreShiftedCell", at.x, at.y);
  }
  auto CopyCellBlock(Point const from, Point const to) -> void override {
    Record("CopyCellBlock", from.x, from.y, to.x, to.y);
  }
  auto OverlayCell([[maybe_unused]] Cell cell, [[maybe_unused]] CellImage image) -> void override {
    Record("OverlayCell");
  }
  auto MaskCell([[maybe_unused]] Cell cell, [[maybe_unused]] CellMask mask) -> void override {
    Record("MaskCell");
  }
  auto DrawCellGlyph([[maybe_unused]] Cell cell, [[maybe_unused]] CellGlyph glyph, Point const at) -> void override {
    Record("DrawCellGlyph", at.x, at.y);
  }
  auto DrawColouredCellGlyph([[maybe_unused]] Cell cell, [[maybe_unused]] CellGlyph glyph, ColouredPlace const place) -> void override {
    Record("DrawColouredCellGlyph", place.at.x, place.at.y, place.colour);
  }
  auto ComposeSprite(Area const area, Sprite const& sprite) -> void override {
    Record("ComposeSprite", area.x, area.y, area.width, area.height, sprite.flags);
  }
  auto ComposeShiftedSprite(Area const area, Sprite const& sprite) -> void override {
    Record("ComposeShiftedSprite", area.x, area.y, area.width, area.height, sprite.flags);
  }
  auto Save(Area const area) -> SnapshotId override {
    Record("Save", area.x, area.y, area.width, area.height);
    return static_cast<SnapshotId>(++_handles);
  }
  auto Restore(SnapshotId const snapshot, Point const at) -> void override {
    Record("Restore", std::to_underlying(snapshot), at.x, at.y);
  }
  auto TakeCell(Point const at) -> CellId override {
    Record("TakeCell", at.x, at.y);
    return static_cast<CellId>(++_handles);
  }
  auto TakeShiftedCell(Point const at) -> CellId override {
    Record("TakeShiftedCell", at.x, at.y);
    return static_cast<CellId>(++_handles);
  }
  auto MaskInCell(CellId const cell, [[maybe_unused]] CellMask mask) -> void override {
    Record("MaskInCell", std::to_underlying(cell));
  }
  auto OverlayInCell(CellId const cell, [[maybe_unused]] CellImage image) -> void override {
    Record("OverlayInCell", std::to_underlying(cell));
  }
  auto GlyphInCell(CellId const cell, [[maybe_unused]] CellGlyph glyph, Point const at) -> void override {
    Record("GlyphInCell", std::to_underlying(cell), at.x, at.y);
  }
  auto ColouredGlyphInCell(CellId const cell, [[maybe_unused]] CellGlyph glyph, ColouredPlace const place) -> void override {
    Record("ColouredGlyphInCell", std::to_underlying(cell), place.at.x, place.at.y, place.colour);
  }
  auto PutCell(Point const at, CellId const cell) -> void override {
    Record("PutCell", at.x, at.y, std::to_underlying(cell));
  }
  auto PutShiftedCell(Point const at, CellId const cell) -> void override {
    Record("PutShiftedCell", at.x, at.y, std::to_underlying(cell));
  }
  auto DrawSprite(Area const area, SpriteImage const& sprite) -> void override {
    Record("DrawSprite", area.x, area.y, area.width, area.height, sprite.flags);
  }
  auto DrawShiftedSprite(Area const area, SpriteImage const& sprite) -> void override {
    Record("DrawShiftedSprite", area.x, area.y, area.width, area.height, sprite.flags);
  }
  auto DrawStamp(Area const area, Stamp const& stamp) -> void override {
    Record("DrawStamp", area.x, area.y, area.width, area.height, stamp.colour);
  }
  auto DrawShiftedStamp(Area const area, Stamp const& stamp) -> void override {
    Record("DrawShiftedStamp", area.x, area.y, area.width, area.height, stamp.colour);
  }
  auto ShowOverlay(Overlay const& overlay) -> OverlayId override {
    Record("ShowOverlay", overlay.shape.index(), overlay.exchange);
    return static_cast<OverlayId>(++_handles);
  }
  auto HideOverlay(OverlayId const overlay) -> void override {
    Record("HideOverlay", std::to_underlying(overlay));
  }
  auto Present() -> void override {
    Record("Present");
  }

private:
  template <typename... _Arguments>
  auto Answer(std::string_view const call, _Arguments const&... arguments) -> bool {
    Record(call, arguments...);
    return _drawn;
  }
  auto Drawing(std::string_view const call, Area const area, Image const image) -> bool {
    return Answer(call, area.x, area.y, area.width, area.height, image.mode, image.bytes.size());
  }

  bool          _drawn{ true };
  std::uint32_t _handles{};
};

// Files in memory, by name; a handle reads and writes at its own position.
class FakeFiles final : public Files, public CallLog {
public:
  auto Store(std::string_view const name, std::vector<std::uint8_t> bytes) -> void {
    _contents[std::string{ name }] = std::move(bytes);
  }
  [[nodiscard]] auto Contents(std::string_view const name) const -> std::optional<std::vector<std::uint8_t>> {
    auto const found{ _contents.find(std::string{ name }) };
    if (found == _contents.end()) return std::nullopt;
    return found->second;
  }

  auto Open(std::string_view const name, [[maybe_unused]] OpenMode mode) -> std::optional<FileHandle> override {
    Record("Open", name);
    if (!_contents.contains(std::string{ name })) return std::nullopt;
    return Opened(name);
  }
  auto Create(std::string_view const name) -> std::optional<FileHandle> override {
    Record("Create", name);
    Store(name, {});
    return Opened(name);
  }
  auto Exists(std::string_view const name) -> bool override {
    Record("Exists", name);
    return _contents.contains(std::string{ name });
  }
  auto Close(FileHandle const file) -> void override {
    Record("Close", std::to_underlying(file));
    _open.erase(file);
  }
  auto Read(FileHandle const file, std::span<std::uint8_t> const into) -> std::optional<std::uint16_t> override {
    auto& [name, position]{ _open.at(file) };
    auto const& bytes{ _contents.at(name) };
    auto const  count{ std::min(into.size(), bytes.size() - std::min(position, bytes.size())) };
    std::ranges::copy(std::span{ bytes }.subspan(position, count), into.begin());
    position += count;
    return static_cast<std::uint16_t>(count);
  }
  auto ReadFar(FileHandle const file, FarRegion const into) -> std::optional<std::uint16_t> override {
    Record("ReadFar", std::to_underlying(file), into.at.segment, into.at.offset, into.size);
    return std::nullopt;
  }
  auto Write(FileHandle const file, std::span<std::uint8_t const> const bytes) -> std::optional<std::uint16_t> override {
    auto& [name, position]{ _open.at(file) };
    auto& contents{ _contents.at(name) };
    contents.resize(std::max(contents.size(), position + bytes.size()));
    std::ranges::copy(bytes, contents.begin() + static_cast<std::ptrdiff_t>(position));
    position += bytes.size();
    return static_cast<std::uint16_t>(bytes.size());
  }
  auto Seek(FileHandle const file, std::int32_t const position, SeekOrigin const origin) -> std::optional<std::int32_t> override {
    auto& [name, at]{ _open.at(file) };
    auto const base{ origin == SeekOrigin::START ? 0 : origin == SeekOrigin::CURRENT ? static_cast<std::int64_t>(at) : static_cast<std::int64_t>(_contents.at(name).size()) };
    if (base + position < 0) return std::nullopt;
    at = static_cast<std::size_t>(base + position);
    return static_cast<std::int32_t>(at);
  }
  auto ResetDisk() -> void override {
    Record("ResetDisk");
  }

private:
  struct OpenFile {
    std::string name;
    std::size_t position{};
  };

  auto Opened(std::string_view const name) -> FileHandle {
    auto const handle{ FileHandle{ _next_handle++ } };
    _open.emplace(handle, OpenFile{ .name = std::string{ name } });
    return handle;
  }

  std::map<std::string, std::vector<std::uint8_t>> _contents;
  std::map<FileHandle, OpenFile>                   _open;
  std::int16_t                                     _next_handle{ 5 }; // DOS hands out 5 first
};

// Keys played back in order; the mouse where the test puts it.
class FakeInput final : public Input, public CallLog {
public:
  auto Press(std::uint16_t const key) -> void {
    _keys.push_back(key);
  }
  auto PlaceMouse(MouseState const state) -> void {
    _mouse = state;
  }
  [[nodiscard]] auto PointerShown() const -> bool {
    return _pointer_shown;
  }

  auto KeyWaiting() -> bool override {
    return !_keys.empty();
  }
  auto ReadKey() -> std::uint16_t override {
    return NextKey();
  }
  auto ReadCharacter() -> std::uint8_t override {
    if (_scan_pending) return static_cast<std::uint8_t>(*std::exchange(_scan_pending, std::nullopt));
    auto const key{ NextKey() };
    if ((key & 0xFF) == 0) _scan_pending = key >> 8;
    return static_cast<std::uint8_t>(key);
  }
  auto ResetMouse() -> bool override {
    Record("ResetMouse");
    return true;
  }
  auto ShowPointer() -> void override {
    _pointer_shown = true;
  }
  auto HidePointer() -> void override {
    _pointer_shown = false;
  }
  auto Mouse() -> MouseState override {
    return _mouse;
  }
  auto MovePointer(Point const to) -> void override {
    _mouse.x = to.x;
    _mouse.y = to.y;
  }
  auto LimitPointerX(Range const columns) -> void override {
    Record("LimitPointerX", columns.min, columns.max);
  }
  auto LimitPointerY(Range const lines) -> void override {
    Record("LimitPointerY", lines.min, lines.max);
  }
  auto Pump() -> void override {
    Record("Pump");
  }

private:
  // The next queued key; none queued is the end of the script (0).
  auto NextKey() -> std::uint16_t {
    if (_keys.empty()) return 0;
    auto const key{ _keys.front() };
    _keys.pop_front();
    return key;
  }

  std::deque<std::uint16_t>    _keys;
  std::optional<std::uint16_t> _scan_pending;
  MouseState                   _mouse;
  bool                         _pointer_shown{};
};

class FakeSound final : public Sound, public CallLog {
public:
  auto Open(FarAddress const music, FarAddress const effects) -> bool override {
    Record("Open", music.segment, music.offset, effects.segment, effects.offset);
    return true;
  }
  auto PlaySong(std::uint8_t const song) -> void override {
    Record("PlaySong", song);
    _music_playing = true;
  }
  auto StopMusic(std::uint8_t const fade) -> void override {
    Record("StopMusic", fade);
    _music_playing = false;
  }
  auto MusicPlaying() -> bool override {
    return _music_playing;
  }
  auto SetRepeatMode(std::uint8_t const mode) -> void override {
    Record("SetRepeatMode", mode);
  }
  auto SetVoiceInstrument(std::uint8_t const voice, FarAddress const instrument) -> void override {
    Record("SetVoiceInstrument", voice, instrument.segment, instrument.offset);
  }
  auto PlayVoiceNote(std::uint8_t const voice, std::uint16_t const note) -> void override {
    Record("PlayVoiceNote", voice, note);
  }
  auto LoadEffects(FarAddress const effects) -> void override {
    Record("LoadEffects", effects.segment, effects.offset);
  }
  auto PlayEffect(std::uint8_t const effect) -> void override {
    Record("PlayEffect", effect);
    _effect_playing = true;
  }
  auto StopEffect() -> void override {
    Record("StopEffect");
    _effect_playing = false;
  }
  auto KeyOffEffectVoice(std::uint8_t const voice) -> void override {
    Record("KeyOffEffectVoice", voice);
  }
  auto EffectPlaying() -> bool override {
    return _effect_playing;
  }
  auto StartCountdown(std::uint16_t const ticks) -> void override {
    Record("StartCountdown", ticks);
  }
  // The countdown is over at once: a test does not wait.
  auto CountdownLeft() -> std::uint16_t override {
    return 0;
  }

private:
  bool _music_playing{};
  bool _effect_playing{};
};

class FakeClock final : public Clock {
public:
  auto Set(std::uint16_t const ticks) -> void {
    _ticks = ticks;
  }
  auto Ticks() -> std::uint16_t override {
    return _ticks;
  }

private:
  std::uint16_t _ticks{};
};

// A machine of fakes.
class FakeMachine {
public:
  [[nodiscard]] auto Machine() noexcept -> services::Machine& {
    return _machine;
  }
  [[nodiscard]] auto Display() noexcept -> FakeDisplay& {
    return _display;
  }
  [[nodiscard]] auto Files() noexcept -> FakeFiles& {
    return _files;
  }
  [[nodiscard]] auto Input() noexcept -> FakeInput& {
    return _input;
  }
  [[nodiscard]] auto Sound() noexcept -> FakeSound& {
    return _sound;
  }
  [[nodiscard]] auto Clock() noexcept -> FakeClock& {
    return _clock;
  }

private:
  FakeDisplay       _display;
  FakeFiles         _files;
  FakeInput         _input;
  FakeSound         _sound;
  FakeClock         _clock;
  services::Machine _machine{
    Devices{ .display = _display, .files = _files, .input = _input, .sound = _sound, .clock = _clock }
  };
};

}
