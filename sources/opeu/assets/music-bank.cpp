#include "opeu/assets/music-bank.hpp"

#include <algorithm>
#include <format>
#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <ranges>
#include <utility>

namespace opeu::assets::detail::music_bank {

namespace {

using Data = std::span<std::uint8_t const>;

constexpr auto RHYTHM_SLOT{ 4uz };
constexpr auto VOICING_BYTES{ 11uz };
constexpr auto EFFECT_FIRST_VOICE{ 4uz };

// Bytes an event takes by its command: melody 00-5Fh a note with length and gate, 60-DFh none (notes with the defaults,
// ignored commands, repeats), E0-F3h one operand, F4-F7h two; rhythm 00-7Fh a drum event with a length (bit 5) and a
// velocity (bit 6), 80-9Fh a velocity, A0-DFh none, E0-F3h one operand, F4-F7h two; F8-FFh none in both.
constexpr auto CommandLength(TrackKind const kind, std::uint8_t const command) -> std::size_t {
  if (kind == TrackKind::MELODY && command <= 0x5F) return 3;
  if (kind == TrackKind::RHYTHM && command < 0x80) return 1uz + ((command & 0x20) != 0 ? 1 : 0) + ((command & 0x40) != 0 ? 1 : 0);
  if (kind == TrackKind::RHYTHM && command < 0xA0) return 2;
  if (command < 0xE0) return 1;
  if (command <= 0xF3) return 2;
  if (command <= 0xF7) return 3;
  return 1;
}

auto StrideOf(BankKind const kind) -> std::size_t {
  return 2 * SlotsOf(kind);
}

auto WordAt(Data const data, std::size_t const at) -> std::size_t {
  return std::size_t{ data[at] } | std::size_t{ data[at + 1] } << 8;
}

auto Wrapped(std::size_t const offset) -> std::size_t {
  return offset & 0xFFFF;
}

// Each offset with its index: where a part lies, and which part it is.
auto IndexByOffset(std::vector<std::size_t> const& offsets) -> std::map<std::size_t, std::size_t> {
  auto const pairs{ std::views::zip(offsets, Indices(offsets.size())) | std::views::transform([](auto const offset_index) { return std::pair{ std::get<0>(offset_index), std::get<1>(offset_index) }; })
                    | std::ranges::to<std::vector>() };
  return { pairs.begin(), pairs.end() }; // not ranges::to: gcc 14's cannot build a map
}

// Where an event lies in the bank.
struct Placed {
  std::size_t at;
  std::size_t length;
};

// A track the table names: where it starts, how it is read, and the first entry and slot that play it.
struct TrackStart {
  std::size_t at;
  TrackKind   kind;
  std::string name;
};

auto TrackName(BankKind const kind, std::size_t const entry, std::size_t const slot) -> std::string {
  if (kind == BankKind::EFFECTS) return std::format("effect-{:02}/voice-{}", entry + 1, slot + EFFECT_FIRST_VOICE);
  if (slot == RHYTHM_SLOT) return std::format("song-{:02}/rhythm", entry + 1);
  return std::format("song-{:02}/voice-{}", entry + 1, slot);
}

auto SlotKind(BankKind const kind, std::size_t const slot) -> TrackKind {
  return kind == BankKind::SONGS && slot == RHYTHM_SLOT ? TrackKind::RHYTHM : TrackKind::MELODY;
}

// The track the table's word at (entry, slot) names, if any.
auto SlotStart(Data const data, BankKind const kind, std::pair<std::size_t, std::size_t> const entry_slot) -> std::optional<TrackStart> {
  auto const [entry, slot]{ entry_slot };
  auto const word{ WordAt(data, (entry * StrideOf(kind)) + (2 * slot)) };
  if (word == 0) return std::nullopt;
  return TrackStart{ .at = Wrapped(word + (entry * StrideOf(kind))), .kind = SlotKind(kind, slot), .name = TrackName(kind, entry, slot) };
}

auto SlotStarts(Data const data, BankLayout const layout) -> std::vector<std::optional<TrackStart>> {
  return Grid(layout.entries, SlotsOf(layout.kind)) | std::views::transform([data, layout](auto const entry_slot) { return SlotStart(data, layout.kind, entry_slot); })
         | std::ranges::to<std::vector>();
}

// Each track once, by its start, named after its first player; none when a track is read both ways.
auto DistinctStarts(std::vector<std::optional<TrackStart>> const& slots) -> std::optional<std::map<std::size_t, TrackStart>> {
  auto starts{ std::map<std::size_t, TrackStart>{} };
  auto used{ slots | std::views::filter([](auto const& slot) { return slot.has_value(); }) | std::views::transform([](auto const& slot) { return *slot; }) };
  std::ranges::for_each(used, [&starts](TrackStart const& start) { starts.try_emplace(start.at, start); });
  auto const consistent{ std::ranges::all_of(used, [&starts](TrackStart const& start) { return starts.at(start.at).kind == start.kind; }) };
  if (!consistent) return std::nullopt;
  return starts;
}

// A track's events from its start to its end command.
auto Disassemble(Data const data, TrackStart const& start) -> Decoded<std::vector<Placed>> {
  auto       events{ std::vector<Placed>{} };
  auto       at{ start.at };
  auto       ended{ false };
  auto       truncated{ false };
  auto const steps{ std::views::iota(0uz) | std::views::take_while([&](std::size_t) { return !ended && !truncated; }) };
  std::ranges::for_each(steps, [&](std::size_t) {
    auto const length{ at < data.size() ? EventLength(start.kind, data.subspan(at)) : std::nullopt };
    if (!length) {
      truncated = true;
      return;
    }
    events.push_back({ .at = at, .length = *length });
    ended  = data[at] == END_COMMAND;
    at    += *length;
  });
  if (truncated) return std::unexpected{ DecodeError::TRUNCATED };
  return events;
}

// A track as the decoder sees it: where it starts and its events.
struct Disassembled {
  TrackStart          start;
  std::vector<Placed> events;
};

auto End(Disassembled const& track) -> std::size_t {
  return track.events.back().at + track.events.back().length;
}

auto SelectTarget(Data const data, Placed const& event) -> std::size_t {
  return Wrapped(WordAt(data, event.at + 1) + event.at);
}

// The distinct offsets the selections of the tracks of one kind point at, ascending, by their index.
auto Targets(Data const data, std::vector<Disassembled> const& tracks, TrackKind const kind) -> std::map<std::size_t, std::size_t> {
  auto offsets{ tracks | std::views::filter([kind](Disassembled const& track) { return track.start.kind == kind; })
                | std::views::transform([](Disassembled const& track) { return std::views::all(track.events); }) | std::views::join
                | std::views::filter([data](Placed const& event) { return data[event.at] == SELECT_COMMAND; })
                | std::views::transform([data](Placed const& event) { return SelectTarget(data, event); }) | std::ranges::to<std::vector>() };
  std::ranges::sort(offsets);
  auto const repeated{ std::ranges::unique(offsets) };
  offsets.erase(repeated.begin(), repeated.end());
  return IndexByOffset(offsets);
}

// Whether pieces of these places and lengths, in this order, follow each other from the start of `whole` to its end.
auto Tiles(std::vector<Placed> const& pieces, Placed const whole) -> bool {
  auto const end{ std::ranges::fold_left(pieces, std::optional<std::size_t>{ whole.at }, [](std::optional<std::size_t> const at, Placed const& piece) {
    return at && *at == piece.at ? std::optional<std::size_t>{ piece.at + piece.length } : std::nullopt;
  }) };
  return end == whole.at + whole.length;
}

auto Pieces(std::map<std::size_t, std::size_t> const& targets, std::size_t const length) -> std::vector<Placed> {
  return targets | std::views::keys | std::views::transform([length](std::size_t const at) { return Placed{ .at = at, .length = length }; }) | std::ranges::to<std::vector>();
}

auto OperatorAt(Data const bytes, std::size_t const which) -> OperatorRegisters {
  return OperatorRegisters{ .character = bytes[which], .waveform = bytes[3 + which], .level = bytes[5 + which], .attack_decay = bytes[7 + which], .sustain_release = bytes[9 + which] };
}

auto VoicingAt(Data const bytes) -> Voicing {
  return Voicing{ .modulator = OperatorAt(bytes, 0), .carrier = OperatorAt(bytes, 1), .connection = bytes[2] };
}

auto InstrumentAt(Data const bytes) -> Instrument {
  return Instrument{
    .voicing     = VoicingAt(bytes.first(VOICING_BYTES)),
    .transpose   = bytes[11],
    .lfo_rate    = static_cast<std::uint32_t>(WordAt(bytes, 12)),
    .lfo_shape   = bytes[14],
    .pitch_depth = bytes[15],
    .depth_step  = bytes[16],
    .level_depth = bytes[17],
    .level_scale = { bytes[18], bytes[19] },
    .lfo_delay   = bytes[20],
    .lfo_enable  = bytes[21]
  };
}

auto DrumChannelAt(Data const bytes) -> DrumChannel {
  return DrumChannel{ .frequency_low = bytes[0], .frequency_high = bytes[1], .voicing = VoicingAt(bytes.subspan(2, VOICING_BYTES)) };
}

auto DrumKitAt(Data const bytes) -> DrumKit {
  constexpr auto CHANNEL_BYTES{ 2 + VOICING_BYTES };
  return { DrumChannelAt(bytes.first(CHANNEL_BYTES)), DrumChannelAt(bytes.subspan(CHANNEL_BYTES, CHANNEL_BYTES)), DrumChannelAt(bytes.subspan(2 * CHANNEL_BYTES, CHANNEL_BYTES)) };
}

// The parts of a bank once its layout is known to tile it.
class BankParts {
public:
  BankParts(Data const data, std::vector<Disassembled> tracks)
      : _data{ data }, _tracks{ std::move(tracks) }, _instruments{ Targets(data, _tracks, TrackKind::MELODY) }, _drum_kits{ Targets(data, _tracks, TrackKind::RHYTHM) } { }

  [[nodiscard]] auto Tiled(std::size_t const table) const -> bool {
    auto const tracks{ _tracks | std::views::transform([](Disassembled const& track) { return Placed{ .at = track.start.at, .length = End(track) - track.start.at }; })
                       | std::ranges::to<std::vector>() };
    auto const pieces{
      std::array{ Pieces(_instruments, INSTRUMENT_BYTES), Pieces(_drum_kits, DRUM_KIT_BYTES), tracks }
      | std::views::join | std::ranges::to<std::vector>()
    };
    return Tiles(pieces, Placed{ .at = table, .length = _data.size() - table });
  }

  [[nodiscard]] auto Bank(BankLayout const layout, std::vector<std::optional<TrackStart>> const& slots) const -> MusicBank {
    auto const index{ IndexByOffset(_tracks | std::views::transform([](Disassembled const& track) { return track.start.at; }) | std::ranges::to<std::vector>()) };
    auto const entry{ [&](std::size_t const n) {
      return slots | std::views::drop(n * SlotsOf(layout.kind)) | std::views::take(SlotsOf(layout.kind))
             | std::views::transform([&index](auto const& slot) { return slot ? std::optional<std::size_t>{ index.at(slot->at) } : std::nullopt; }) | std::ranges::to<Entry>();
    } };
    return MusicBank{ .kind        = layout.kind,
                      .entries     = Indices(layout.entries) | std::views::transform(entry) | std::ranges::to<std::vector>(),
                      .instruments = _instruments | std::views::keys | std::views::transform([this](std::size_t const at) { return InstrumentAt(_data.subspan(at, INSTRUMENT_BYTES)); })
                                     | std::ranges::to<std::vector>(),
                      .drum_kits = _drum_kits | std::views::keys | std::views::transform([this](std::size_t const at) { return DrumKitAt(_data.subspan(at, DRUM_KIT_BYTES)); })
                                   | std::ranges::to<std::vector>(),
                      .tracks = _tracks | std::views::transform([this](Disassembled const& track) { return TrackOf(track); }) | std::ranges::to<std::vector>() };
  }

private:
  [[nodiscard]] auto EventOf(TrackKind const kind, Placed const& event) const -> Event {
    if (_data[event.at] != SELECT_COMMAND) return Event{ .bytes = _data.subspan(event.at, event.length) | std::ranges::to<std::vector>(), .selects = std::nullopt };
    auto const& targets{ kind == TrackKind::MELODY ? _instruments : _drum_kits };
    return Event{ .bytes = { SELECT_COMMAND }, .selects = targets.at(SelectTarget(_data, event)) };
  }

  [[nodiscard]] auto TrackOf(Disassembled const& track) const -> Track {
    return Track{ .name   = track.start.name,
                  .kind   = track.start.kind,
                  .events = track.events | std::views::transform([this, &track](Placed const& event) { return EventOf(track.start.kind, event); }) | std::ranges::to<std::vector>() };
  }

  Data                               _data;
  std::vector<Disassembled>          _tracks;
  std::map<std::size_t, std::size_t> _instruments;
  std::map<std::size_t, std::size_t> _drum_kits;
};

auto DisassembleAll(Data const data, std::map<std::size_t, TrackStart> const& starts) -> Decoded<std::vector<Disassembled>> {
  auto tracks{ std::vector<Disassembled>{} };
  auto failure{ std::optional<DecodeError>{} };
  std::ranges::for_each(starts | std::views::values, [&](TrackStart const& start) {
    if (failure) return;
    auto events{ Disassemble(data, start) };
    if (!events)
      failure = events.error();
    else
      tracks.push_back({ .start = start, .events = *std::move(events) });
  });
  if (failure) return std::unexpected{ *failure };
  return tracks;
}

// ---- encoding ----

auto WordBytes(std::size_t const word) -> std::array<std::uint8_t, 2> {
  return { static_cast<std::uint8_t>(word & 0xFF), static_cast<std::uint8_t>(word >> 8 & 0xFF) };
}

auto VoicingBytes(Voicing const& v) -> std::array<std::uint8_t, VOICING_BYTES> {
  return { v.modulator.character, v.carrier.character,      v.connection,           v.modulator.waveform,        v.carrier.waveform,       v.modulator.level,
           v.carrier.level,       v.modulator.attack_decay, v.carrier.attack_decay, v.modulator.sustain_release, v.carrier.sustain_release };
}

auto InstrumentBytes(Instrument const& instrument) -> std::vector<std::uint8_t> {
  auto const rate{ WordBytes(instrument.lfo_rate) };
  auto const lfo{
    std::array{ instrument.transpose, rate[0], rate[1], instrument.lfo_shape, instrument.pitch_depth, instrument.depth_step, instrument.level_depth, instrument.level_scale[0],
               instrument.level_scale[1], instrument.lfo_delay, instrument.lfo_enable }
  };
  return std::array{ VoicingBytes(instrument.voicing) | std::ranges::to<std::vector>(), lfo | std::ranges::to<std::vector>() } | std::views::join | std::ranges::to<std::vector>();
}

auto DrumChannelBytes(DrumChannel const& channel) -> std::vector<std::uint8_t> {
  auto const frequency{
    std::array{ channel.frequency_low, channel.frequency_high }
  };
  return std::array{ frequency | std::ranges::to<std::vector>(), VoicingBytes(channel.voicing) | std::ranges::to<std::vector>() } | std::views::join | std::ranges::to<std::vector>();
}

auto DrumKitBytes(DrumKit const& kit) -> std::vector<std::uint8_t> {
  return kit | std::views::transform(DrumChannelBytes) | std::views::join | std::ranges::to<std::vector>();
}

auto EncodedLength(Event const& event) -> std::size_t {
  return event.bytes.size() + (event.selects ? 2 : 0);
}

// Where pieces of these lengths start when laid end to end from `at`.
auto Starts(std::vector<std::size_t> const& lengths, std::size_t const at) -> std::vector<std::size_t> {
  auto starts{ std::vector<std::size_t>(lengths.size()) };
  std::exclusive_scan(lengths.begin(), lengths.end(), starts.begin(), at);
  return starts;
}

auto EventStarts(std::vector<Event> const& events, std::size_t const at) -> std::vector<std::size_t> {
  return Starts(events | std::views::transform(EncodedLength) | std::ranges::to<std::vector>(), at);
}

auto TrackLength(Track const& track) -> std::size_t {
  return std::ranges::fold_left(track.events | std::views::transform(EncodedLength), 0uz, std::plus{});
}

// A bank laid out: where each part lands, and the bytes of each part with its offsets worked out.
class BankEncoder {
public:
  explicit BankEncoder(MusicBank const& bank)
      : _bank{ bank }, _table{ bank.entries.size() * StrideOf(bank.kind) }, _drum_kits_at{ _table + (bank.instruments.size() * INSTRUMENT_BYTES) },
        _track_offsets{ Starts(bank.tracks | std::views::transform(TrackLength) | std::ranges::to<std::vector>(), _drum_kits_at + (bank.drum_kits.size() * DRUM_KIT_BYTES)) } { }

  [[nodiscard]] auto Bytes() const -> std::vector<std::uint8_t> {
    auto const& bank{ _bank.get() };
    auto const  parts{
      std::array{
                 TableBytes(),
                 bank.instruments | std::views::transform(InstrumentBytes) | std::views::join | std::ranges::to<std::vector>(),
                 bank.drum_kits | std::views::transform(DrumKitBytes) | std::views::join | std::ranges::to<std::vector>(),
                 Indices(bank.tracks.size()) | std::views::transform([this](std::size_t const n) { return TrackBytes(n); }) | std::views::join | std::ranges::to<std::vector>(),
                 }
    };
    return parts | std::views::join | std::ranges::to<std::vector>();
  }

private:
  [[nodiscard]] auto TableBytes() const -> std::vector<std::uint8_t> {
    auto const& bank{ _bank.get() };
    auto const  stride{ StrideOf(bank.kind) };
    return std::views::zip(bank.entries, Indices(bank.entries.size())) | std::views::transform([this, stride](auto const entry_index) {
             auto const& [entry, n]{ entry_index };
             return entry | std::views::transform([this, stride, n](std::optional<std::size_t> const track) { return WordBytes(track ? Wrapped(_track_offsets.at(*track) - (n * stride)) : 0); })
                    | std::views::join | std::ranges::to<std::vector>();
           })
           | std::views::join | std::ranges::to<std::vector>();
  }

  [[nodiscard]] auto TrackBytes(std::size_t const n) const -> std::vector<std::uint8_t> {
    auto const& track{ _bank.get().tracks.at(n) };
    return std::views::zip(track.events, EventStarts(track.events, _track_offsets.at(n)))
           | std::views::transform([this, &track](auto const event_at) { return EventBytes(track.kind, std::get<0>(event_at), std::get<1>(event_at)); }) | std::views::join
           | std::ranges::to<std::vector>();
  }

  // An event's bytes; a selection's operand is its target's offset relative to the command at `at`.
  [[nodiscard]] auto EventBytes(TrackKind const kind, Event const& event, std::size_t const at) const -> std::vector<std::uint8_t> {
    if (!event.selects) return event.bytes;
    auto const target{ kind == TrackKind::MELODY ? _table + (*event.selects * INSTRUMENT_BYTES) : _drum_kits_at + (*event.selects * DRUM_KIT_BYTES) };
    return std::array{ event.bytes, WordBytes(Wrapped(target - at)) | std::ranges::to<std::vector>() } | std::views::join | std::ranges::to<std::vector>();
  }

  std::reference_wrapper<MusicBank const> _bank;
  std::size_t                             _table;
  std::size_t                             _drum_kits_at;
  std::vector<std::size_t>                _track_offsets;
};

}

auto EventLength(TrackKind const kind, std::span<std::uint8_t const> const rest) -> std::optional<std::size_t> {
  if (rest.empty()) return std::nullopt;
  auto const length{ CommandLength(kind, rest.front()) };
  if (length > rest.size()) return std::nullopt;
  return length;
}

auto DecodeMusicBank(Bytes const bytes, BankLayout const layout) -> Decoded<MusicBank> {
  auto const owned{ bytes | std::views::transform([](std::byte const b) { return std::to_integer<std::uint8_t>(b); }) | std::ranges::to<std::vector>() };
  auto const data{ Data{ owned } };
  auto const table{ layout.entries * StrideOf(layout.kind) };
  if (data.size() < table) return std::unexpected{ DecodeError::TRUNCATED };
  auto const slots{ SlotStarts(data, layout) };
  auto const starts{ DistinctStarts(slots) };
  if (!starts) return std::unexpected{ DecodeError::BAD_LAYOUT };
  return DisassembleAll(data, *starts).and_then([&](std::vector<Disassembled> tracks) -> Decoded<MusicBank> {
    auto const parts{
      BankParts{ data, std::move(tracks) }
    };
    if (!parts.Tiled(table)) return std::unexpected{ DecodeError::BAD_LAYOUT };
    return parts.Bank(layout, slots);
  });
}

auto EncodeMusicBank(MusicBank const& bank) -> std::vector<std::byte> {
  return BankEncoder{ bank }.Bytes() | std::views::transform([](std::uint8_t const b) { return std::byte{ b }; }) | std::ranges::to<std::vector>();
}

}
