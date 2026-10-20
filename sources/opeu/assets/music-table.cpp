#include "opeu/assets/music-table.hpp"

#include "opeu/assets/table-json.hpp"

#include <algorithm>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace opeu::assets::detail::music_table {

namespace {

using namespace table_json;
using Ordered = nlohmann::ordered_json; // written in the order a reader expects; read back as Json
using Names   = std::map<std::string, std::size_t, std::less<>>;

constexpr auto MUSIC_BANK{ "music_bank"sv };
constexpr auto NOTE_NAMES{ std::to_array<std::string_view>({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }) };
constexpr auto LFO_SHAPES{ std::to_array<std::string_view>({ "sawtooth", "square", "triangle", "random", "ramp" }) };
constexpr auto DRUM_NAMES{ std::to_array<std::string_view>({ "hi-hat", "cymbal", "tom", "snare", "bass-drum" }) }; // bits 0-4
constexpr auto KIT_CHANNELS{ std::to_array<std::string_view>({ "bass_drum", "hi_hat_snare", "tom_cymbal" }) };
constexpr auto SONG_SLOTS{ std::to_array<std::string_view>({ "voice-0", "voice-1", "voice-2", "voice-3", "rhythm" }) };
constexpr auto EFFECT_SLOTS{ std::to_array<std::string_view>({ "voice-4", "voice-5" }) };
constexpr auto LAST_NOTE{ std::uint8_t{ 0x5F } };
constexpr auto DEFAULT_NOTES{ std::uint8_t{ 0x60 } }; // 60-BFh: a note with the default length and gate
constexpr auto REPEAT_START{ std::uint8_t{ 0xD0 } };
constexpr auto LENGTH_FOLLOWS{ std::uint8_t{ 0x20 } };
constexpr auto VELOCITY_FOLLOWS{ std::uint8_t{ 0x40 } };
constexpr auto DRUM_VELOCITY{ std::uint8_t{ 0x80 } };

// The commands both kinds of track name alike, and the melody's and the rhythm's own.
struct NamedCommand {
  std::uint8_t     command;
  std::string_view op;
  std::string_view operand; // empty: none
};
constexpr auto FLOW_COMMANDS{
  std::to_array<NamedCommand>({
                               { .command = 0xF0, .op = "volume", .operand = "value" },
                               { .command = 0xF2, .op = "tempo", .operand = "bpm" },
                               { .command = 0xFD, .op = "repeat-break", .operand = "" },
                               { .command = 0xFE, .op = "loop-point", .operand = "" },
                               { .command = 0xFF, .op = "end", .operand = "" },
                               }
  )
};
constexpr auto MELODY_COMMANDS{
  std::to_array<NamedCommand>({
                               { .command = 0xEE, .op = "am-depth", .operand = "value" },
                               { .command = 0xEF, .op = "vibrato-depth", .operand = "value" },
                               { .command = 0xF1, .op = "tempo-period", .operand = "value" },
                               { .command = 0xF8, .op = "lfo-on", .operand = "" },
                               { .command = 0xF9, .op = "lfo-off", .operand = "" },
                               }
  )
};
constexpr auto MELODY_VIBRATO{
  std::to_array<NamedCommand>({
                               { .command = 0xF6, .op = "vibrato", .operand = "tempo_rate" },
                               { .command = 0xF7, .op = "vibrato", .operand = "rate" },
                               }
  )
};

auto Bit(std::uint8_t const value, std::uint8_t const mask) -> bool {
  return (value & mask) != 0;
}

auto Signed(std::uint8_t const value) -> int {
  return static_cast<std::int8_t>(value);
}

auto NoteName(std::uint8_t const note) -> std::string {
  return std::format("{}{}", NOTE_NAMES.at(note % NOTE_NAMES.size()), note / NOTE_NAMES.size());
}

auto InstrumentName(std::size_t const n) -> std::string {
  return std::format("instrument-{:02}", n);
}

auto DrumKitName(std::size_t const n) -> std::string {
  return std::format("drum-kit-{:02}", n);
}

auto SlotNames(BankKind const kind) -> std::span<std::string_view const> {
  if (kind == BankKind::SONGS) return SONG_SLOTS;
  return EFFECT_SLOTS;
}

auto EntryPrefix(BankKind const kind) -> std::string_view {
  return kind == BankKind::SONGS ? "song" : "effect";
}

auto Find(std::span<NamedCommand const> const commands, std::uint8_t const command) -> std::optional<NamedCommand> {
  auto const found{ std::ranges::find(commands, command, &NamedCommand::command) };
  if (found == commands.end()) return std::nullopt;
  return *found;
}

// ---- writing ----

auto OperatorJson(music_bank::OperatorRegisters const& r) -> Ordered {
  auto json               = Ordered::object();
  json["tremolo"]         = Bit(r.character, 0x80);
  json["vibrato"]         = Bit(r.character, 0x40);
  json["sustained"]       = Bit(r.character, 0x20);
  json["key_scale_rate"]  = Bit(r.character, 0x10);
  json["multiple"]        = r.character & 0x0F;
  json["waveform"]        = r.waveform;
  json["key_scale_level"] = r.level >> 6;
  json["level"]           = r.level & 0x3F;
  json["attack"]          = r.attack_decay >> 4;
  json["decay"]           = r.attack_decay & 0x0F;
  json["sustain"]         = r.sustain_release >> 4;
  json["release"]         = r.sustain_release & 0x0F;
  return json;
}

auto VoicingJson(music_bank::Voicing const& voicing) -> Ordered {
  auto json         = Ordered::object();
  json["modulator"] = OperatorJson(voicing.modulator);
  json["carrier"]   = OperatorJson(voicing.carrier);
  json["feedback"]  = voicing.connection >> 1;
  json["additive"]  = Bit(voicing.connection, 0x01);
  return json;
}

auto ShapeJson(std::uint8_t const shape) -> Ordered {
  if (shape < LFO_SHAPES.size()) return LFO_SHAPES.at(shape);
  return shape;
}

auto InstrumentJson(Instrument const& instrument, std::size_t const n) -> Ordered {
  auto lfo           = Ordered::object();
  lfo["enabled"]     = instrument.lfo_enable;
  lfo["shape"]       = ShapeJson(instrument.lfo_shape);
  lfo["rate"]        = instrument.lfo_rate;
  lfo["delay"]       = instrument.lfo_delay;
  lfo["pitch_depth"] = Signed(instrument.pitch_depth);
  lfo["depth_step"]  = instrument.depth_step;
  lfo["level_depth"] = Signed(instrument.level_depth);
  lfo["level_scale"] = instrument.level_scale;
  auto json          = Ordered::object();
  json["name"]       = InstrumentName(n);
  json["voicing"]    = VoicingJson(instrument.voicing);
  json["transpose"]  = Signed(instrument.transpose);
  json["lfo"]        = std::move(lfo);
  return json;
}

auto DrumKitJson(DrumKit const& kit, std::size_t const n) -> Ordered {
  auto json    = Ordered::object();
  json["name"] = DrumKitName(n);
  std::ranges::for_each(std::views::zip(KIT_CHANNELS, kit), [&json](auto const name_channel) {
    auto const& [name, channel]{ name_channel };
    auto record       = Ordered::object();
    record["block"]   = channel.frequency_high >> 2;
    record["fnumber"] = ((channel.frequency_high & 0x03) << 8) | channel.frequency_low;
    record["voicing"] = VoicingJson(channel.voicing);
    json[name]        = std::move(record);
  });
  return json;
}

auto EntryJson(MusicBank const& bank, std::size_t const n) -> Ordered {
  auto json    = Ordered::object();
  json["name"] = std::format("{}-{:02}", EntryPrefix(bank.kind), n + 1);
  std::ranges::for_each(std::views::zip(SlotNames(bank.kind), bank.entries.at(n)), [&](auto const name_track) {
    auto const& [name, track]{ name_track };
    json[name] = track ? Ordered(bank.tracks.at(*track).name) : Ordered(nullptr);
  });
  return json;
}

auto DrumsJson(std::uint8_t const command) -> Ordered {
  return std::views::zip(DRUM_NAMES, Indices(DRUM_NAMES.size())) | std::views::filter([command](auto const name_bit) { return Bit(command, static_cast<std::uint8_t>(1U << std::get<1>(name_bit))); })
         | std::views::keys | std::ranges::to<std::vector<std::string_view>>();
}

auto Op(std::string_view const op) -> Ordered {
  auto json  = Ordered::object();
  json["op"] = op;
  return json;
}

auto NamedJson(NamedCommand const& named, std::vector<std::uint8_t> const& bytes) -> Ordered {
  auto json = Op(named.op);
  if (!named.operand.empty()) json[named.operand] = bytes.at(1);
  return json;
}

auto IgnoredJson(std::vector<std::uint8_t> const& bytes) -> Ordered {
  auto json        = Op("ignored");
  json["command"]  = bytes.front();
  json["operands"] = bytes | std::views::drop(1) | std::ranges::to<std::vector>();
  return json;
}

// The events both kinds of track share: the repeats and the flow commands.
auto SharedJson(std::vector<std::uint8_t> const& bytes) -> std::optional<Ordered> {
  auto const command{ bytes.front() };
  if (command == REPEAT_START) return Op("repeat-start");
  if (command > REPEAT_START && command <= (REPEAT_START | 0x0F)) {
    auto json          = Op("repeat");
    json["more_times"] = command & 0x0F;
    return json;
  }
  if (auto const named{ Find(FLOW_COMMANDS, command) }) return NamedJson(*named, bytes);
  return std::nullopt;
}

auto MelodyJson(std::vector<std::uint8_t> const& bytes) -> Ordered {
  auto const command{ bytes.front() };
  if (command < 2 * DEFAULT_NOTES) {
    auto json    = Op("note");
    json["note"] = NoteName(command <= LAST_NOTE ? command : static_cast<std::uint8_t>(command - DEFAULT_NOTES));
    if (command <= LAST_NOTE) {
      json["length"] = bytes.at(1);
      json["gate"]   = bytes.at(2);
    }
    return json;
  }
  if (auto shared{ SharedJson(bytes) }) return *std::move(shared);
  if (auto const named{ Find(MELODY_COMMANDS, command) }) return NamedJson(*named, bytes);
  if (auto const vibrato{ Find(MELODY_VIBRATO, command) }) {
    auto json              = Op(vibrato->op);
    json["depth"]          = Signed(bytes.at(1));
    json[vibrato->operand] = bytes.at(2);
    return json;
  }
  return IgnoredJson(bytes);
}

auto RhythmJson(std::vector<std::uint8_t> const& bytes) -> Ordered {
  auto const command{ bytes.front() };
  if (command < DRUM_VELOCITY) {
    auto json     = Op("drums");
    json["drums"] = DrumsJson(command);
    if (Bit(command, LENGTH_FOLLOWS)) json["length"] = bytes.at(1);
    if (Bit(command, VELOCITY_FOLLOWS)) json["velocity"] = bytes.back();
    return json;
  }
  if (command < DRUM_VELOCITY + 0x20) {
    auto json        = Op("drum-velocity");
    json["drums"]    = DrumsJson(command);
    json["velocity"] = bytes.at(1);
    return json;
  }
  if (auto shared{ SharedJson(bytes) }) return *std::move(shared);
  return IgnoredJson(bytes);
}

auto EventJson(TrackKind const kind, Event const& event) -> Ordered {
  if (event.selects) {
    auto json    = Op(kind == TrackKind::MELODY ? "instrument" : "drum-kit");
    json["name"] = kind == TrackKind::MELODY ? InstrumentName(*event.selects) : DrumKitName(*event.selects);
    return json;
  }
  return kind == TrackKind::MELODY ? MelodyJson(event.bytes) : RhythmJson(event.bytes);
}

auto TrackJson(Track const& track) -> Ordered {
  auto json      = Ordered::object();
  json["name"]   = track.name;
  json["kind"]   = track.kind == TrackKind::MELODY ? "melody" : "rhythm";
  json["events"] = track.events | std::views::transform([&track](Event const& event) { return EventJson(track.kind, event); }) | std::ranges::to<std::vector>();
  return json;
}

// ---- reading ----

auto Flag(Json const& record, std::string_view const key) -> std::uint8_t {
  return record.at(std::string{ key }).get<bool>() ? 1 : 0;
}

auto SignedByte(Json const& record, std::string_view const key) -> std::uint8_t {
  auto const value{ record.at(std::string{ key }).get<int>() };
  if (value < -0x80 || value > 0x7F) throw std::out_of_range{ std::format("{} {} is not a byte from -128 to 127", key, value) };
  return static_cast<std::uint8_t>(value);
}

auto Field(Json const& record, std::string_view const key, std::uint32_t const limit) -> std::uint8_t {
  return static_cast<std::uint8_t>(Number(record, key, limit));
}

auto OperatorOf(Json const& json) -> music_bank::OperatorRegisters {
  return { .character       = static_cast<std::uint8_t>((Flag(json, "tremolo") << 7) | (Flag(json, "vibrato") << 6) | (Flag(json, "sustained") << 5) | (Flag(json, "key_scale_rate") << 4)
                                                        | Field(json, "multiple", 0x0F)),
           .waveform        = ByteField(json, "waveform"),
           .level           = static_cast<std::uint8_t>((Field(json, "key_scale_level", 0x03) << 6) | Field(json, "level", 0x3F)),
           .attack_decay    = static_cast<std::uint8_t>((Field(json, "attack", 0x0F) << 4) | Field(json, "decay", 0x0F)),
           .sustain_release = static_cast<std::uint8_t>((Field(json, "sustain", 0x0F) << 4) | Field(json, "release", 0x0F)) };
}

auto VoicingOf(Json const& json) -> music_bank::Voicing {
  return { .modulator  = OperatorOf(json.at("modulator")),
           .carrier    = OperatorOf(json.at("carrier")),
           .connection = static_cast<std::uint8_t>((Field(json, "feedback", 0x7F) << 1) | Flag(json, "additive")) };
}

// The position of a name in a list of names, refused when it is none of them.
auto PositionOf(std::span<std::string_view const> const names, std::string_view const name, std::string_view const what) -> std::size_t {
  auto const found{ std::ranges::find(names, name) };
  if (found == names.end()) throw std::invalid_argument{ std::format("no {} {}", what, name) };
  return static_cast<std::size_t>(found - names.begin());
}

auto ShapeOf(Json const& lfo) -> std::uint8_t {
  auto const& shape{ lfo.at("shape") };
  if (!shape.is_string()) return ByteField(lfo, "shape");
  return static_cast<std::uint8_t>(PositionOf(LFO_SHAPES, shape.get<std::string>(), "LFO shape"));
}

auto InstrumentOf(Json const& json) -> Instrument {
  auto const& lfo{ json.at("lfo") };
  auto const  scale{ Numbers<2>(lfo, "level_scale", 0xFF) };
  return Instrument{
    .voicing     = VoicingOf(json.at("voicing")),
    .transpose   = SignedByte(json, "transpose"),
    .lfo_rate    = WordField(lfo, "rate"),
    .lfo_shape   = ShapeOf(lfo),
    .pitch_depth = SignedByte(lfo, "pitch_depth"),
    .depth_step  = ByteField(lfo, "depth_step"),
    .level_depth = SignedByte(lfo, "level_depth"),
    .level_scale = { static_cast<std::uint8_t>(scale[0]), static_cast<std::uint8_t>(scale[1]) },
    .lfo_delay   = ByteField(lfo, "delay"),
    .lfo_enable  = ByteField(lfo, "enabled")
  };
}

auto DrumChannelOf(Json const& json) -> DrumChannel {
  auto const fnumber{ Number(json, "fnumber", 0x3FF) };
  return DrumChannel{ .frequency_low  = static_cast<std::uint8_t>(fnumber & 0xFF),
                      .frequency_high = static_cast<std::uint8_t>((Number(json, "block", 0x3F) << 2) | (fnumber >> 8)),
                      .voicing        = VoicingOf(json.at("voicing")) };
}

auto DrumKitOf(Json const& json) -> DrumKit {
  return { DrumChannelOf(json.at("bass_drum")), DrumChannelOf(json.at("hi_hat_snare")), DrumChannelOf(json.at("tom_cymbal")) };
}

// The records' names by their index; refuses a name twice.
auto NamesOf(Json const& records) -> Names {
  auto const names{ records | std::views::transform([](Json const& record) { return record.at("name").get<std::string>(); }) | std::ranges::to<std::vector>() };
  auto const pairs{ std::views::zip(names, Indices(names.size())) | std::views::transform([](auto const name_n) { return std::pair{ std::get<0>(name_n), std::get<1>(name_n) }; })
                    | std::ranges::to<std::vector>() };
  auto const index{ Names(pairs.begin(), pairs.end()) }; // not ranges::to: gcc 14's cannot build a map
  if (index.size() != names.size()) throw std::invalid_argument{ "a name appears twice" };
  return index;
}

auto IndexOf(Names const& names, std::string const& name) -> std::size_t {
  auto const found{ names.find(name) };
  if (found == names.end()) throw std::invalid_argument{ std::format("nothing is named {}", name) };
  return found->second;
}

auto NoteOf(Json const& json) -> std::uint8_t {
  auto const name{ json.at("note").get<std::string>() };
  auto const octave_at{ name.find_first_of("0123456789") };
  if (octave_at == std::string::npos || octave_at + 1 != name.size()) throw std::invalid_argument{ std::format("no note {}", name) };
  auto const pitch{ PositionOf(NOTE_NAMES, std::string_view{ name }.substr(0, octave_at), "note") };
  auto const note{ (static_cast<std::size_t>(name.back() - '0') * NOTE_NAMES.size()) + pitch };
  if (note > LAST_NOTE) throw std::out_of_range{ std::format("note {} above the top", name) };
  return static_cast<std::uint8_t>(note);
}

auto DrumBits(Json const& json) -> std::uint8_t {
  return std::ranges::fold_left(json.at("drums"), std::uint8_t{},
                                [](std::uint8_t const bits, Json const& drum) { return static_cast<std::uint8_t>(bits | (1U << PositionOf(DRUM_NAMES, drum.get<std::string>(), "drum"))); });
}

auto NamedOf(Json const& json, NamedCommand const& named) -> std::vector<std::uint8_t> {
  if (named.operand.empty()) return { named.command };
  return { named.command, ByteField(json, named.operand) };
}

auto FindOp(std::span<NamedCommand const> const commands, std::string_view const op) -> std::optional<NamedCommand> {
  auto const found{ std::ranges::find(commands, op, &NamedCommand::op) };
  if (found == commands.end()) return std::nullopt;
  return *found;
}

auto IgnoredOf(Json const& json) -> std::vector<std::uint8_t> {
  auto bytes{ std::vector{ ByteField(json, "command") } };
  std::ranges::for_each(json.at("operands"), [&bytes](Json const& operand) {
    auto const value{ operand.get<std::uint32_t>() };
    if (value > 0xFF) throw std::out_of_range{ std::format("operand {} is no byte", value) };
    bytes.push_back(static_cast<std::uint8_t>(value));
  });
  return bytes;
}

auto SharedOf(Json const& json, std::string_view const op) -> std::optional<std::vector<std::uint8_t>> {
  if (op == "repeat-start") return std::vector{ REPEAT_START };
  if (op == "repeat") return std::vector{ static_cast<std::uint8_t>(REPEAT_START | Field(json, "more_times", 0x0F)) };
  if (op == "ignored") return IgnoredOf(json);
  if (auto const named{ FindOp(FLOW_COMMANDS, op) }) return NamedOf(json, *named);
  return std::nullopt;
}

auto MelodyOf(Json const& json, std::string_view const op) -> std::vector<std::uint8_t> {
  if (op == "note" && json.contains("length")) return { NoteOf(json), ByteField(json, "length"), ByteField(json, "gate") };
  if (op == "note") return { static_cast<std::uint8_t>(NoteOf(json) + DEFAULT_NOTES) };
  if (op == "vibrato") {
    auto const tempo{ json.contains("tempo_rate") };
    return { MELODY_VIBRATO.at(tempo ? 0 : 1).command, SignedByte(json, "depth"), ByteField(json, tempo ? "tempo_rate" : "rate") };
  }
  if (auto const named{ FindOp(MELODY_COMMANDS, op) }) return NamedOf(json, *named);
  if (auto shared{ SharedOf(json, op) }) return *std::move(shared);
  throw std::invalid_argument{ std::format("no melody event {}", op) };
}

auto RhythmOf(Json const& json, std::string_view const op) -> std::vector<std::uint8_t> {
  if (op == "drums") {
    auto const length{ json.contains("length") };
    auto const velocity{ json.contains("velocity") };
    auto       bytes{ std::vector{ static_cast<std::uint8_t>(DrumBits(json) | (length ? LENGTH_FOLLOWS : 0) | (velocity ? VELOCITY_FOLLOWS : 0)) } };
    if (length) bytes.push_back(ByteField(json, "length"));
    if (velocity) bytes.push_back(ByteField(json, "velocity"));
    return bytes;
  }
  if (op == "drum-velocity") return { static_cast<std::uint8_t>(DRUM_VELOCITY | DrumBits(json)), ByteField(json, "velocity") };
  if (auto shared{ SharedOf(json, op) }) return *std::move(shared);
  throw std::invalid_argument{ std::format("no rhythm event {}", op) };
}

// The names an event of each kind of track may select.
struct Selectable {
  Names instruments;
  Names drum_kits;
};

auto EventOf(Json const& json, TrackKind const kind, Selectable const& selectable) -> Event {
  auto const op{ json.at("op").get<std::string>() };
  if (op == (kind == TrackKind::MELODY ? "instrument" : "drum-kit")) {
    return Event{ .bytes = { music_bank::SELECT_COMMAND }, .selects = IndexOf(kind == TrackKind::MELODY ? selectable.instruments : selectable.drum_kits, json.at("name").get<std::string>()) };
  }
  auto bytes{ kind == TrackKind::MELODY ? MelodyOf(json, op) : RhythmOf(json, op) };
  if (bytes.front() == music_bank::SELECT_COMMAND || music_bank::EventLength(kind, bytes) != bytes.size()) throw std::invalid_argument{ std::format("{} has the wrong operands", json.dump()) };
  return Event{ .bytes = std::move(bytes), .selects = std::nullopt };
}

auto TrackOf(Json const& json, Selectable const& selectable) -> Track {
  auto const kind_name{ json.at("kind").get<std::string>() };
  if (kind_name != "melody" && kind_name != "rhythm") throw std::invalid_argument{ std::format("no track kind {}", kind_name) };
  auto const kind{ kind_name == "melody" ? TrackKind::MELODY : TrackKind::RHYTHM };
  auto       events{ json.at("events") | std::views::transform([kind, &selectable](Json const& event) { return EventOf(event, kind, selectable); }) | std::ranges::to<std::vector>() };
  if (events.empty() || events.back().bytes != std::vector{ music_bank::END_COMMAND }) throw std::invalid_argument{ std::format("{} does not end with end", json.at("name").dump()) };
  return Track{ .name = json.at("name").get<std::string>(), .kind = kind, .events = std::move(events) };
}

auto EntryOf(Json const& json, BankKind const kind, Names const& tracks) -> music_bank::Entry {
  return SlotNames(kind) | std::views::transform([&](std::string_view const slot) {
           auto const& track{ json.at(std::string{ slot }) };
           return track.is_null() ? std::nullopt : std::optional<std::size_t>{ IndexOf(tracks, track.get<std::string>()) };
         })
         | std::ranges::to<music_bank::Entry>();
}

// A track must be read the way its slots play it: a song's rhythm slot plays a rhythm track, every other slot a melody.
auto CheckSlots(MusicBank const& bank) -> void {
  auto const fits{ std::ranges::all_of(bank.entries, [&bank](music_bank::Entry const& entry) {
    return std::ranges::all_of(std::views::zip(entry, Indices(entry.size())), [&bank](auto const track_slot) {
      auto const [track, slot]{ track_slot };
      auto const rhythm{ bank.kind == BankKind::SONGS && slot == SONG_SLOTS.size() - 1 };
      return !track || (bank.tracks.at(*track).kind == TrackKind::RHYTHM) == rhythm;
    });
  }) };
  if (!fits) throw std::invalid_argument{ "a slot plays a track of the other kind" };
}

auto MusicBankOf(Json const& json) -> MusicBank {
  auto const kind_name{ json.at("kind").get<std::string>() };
  if (kind_name != "songs" && kind_name != "effects") throw std::invalid_argument{ std::format("no bank kind {}", kind_name) };
  auto const  kind{ kind_name == "songs" ? BankKind::SONGS : BankKind::EFFECTS };
  auto const& tracks{ json.at("tracks") };
  auto const  selectable{
    Selectable{ .instruments = NamesOf(json.at("instruments")), .drum_kits = NamesOf(json.at("drum_kits")) }
  };
  auto const track_names{ NamesOf(tracks) };
  auto       bank{
    MusicBank{ .kind    = kind,
              .entries = json.at(kind_name) | std::views::transform([kind, &track_names](Json const& entry) { return EntryOf(entry, kind, track_names); }) | std::ranges::to<std::vector>(),
              .instruments = json.at("instruments") | std::views::transform(InstrumentOf) | std::ranges::to<std::vector>(),
              .drum_kits   = json.at("drum_kits") | std::views::transform(DrumKitOf) | std::ranges::to<std::vector>(),
              .tracks      = tracks | std::views::transform([&selectable](Json const& track) { return TrackOf(track, selectable); }) | std::ranges::to<std::vector>() }
  };
  CheckSlots(bank);
  return bank;
}

}

auto MusicBankText(MusicBank const& bank) -> std::string {
  auto const kind_name{ std::string_view{ bank.kind == BankKind::SONGS ? "songs" : "effects" } };
  auto       json     = Ordered::object();
  json["kind"]        = kind_name;
  json[kind_name]     = Indices(bank.entries.size()) | std::views::transform([&bank](std::size_t const n) { return EntryJson(bank, n); }) | std::ranges::to<std::vector>();
  json["instruments"] = std::views::zip(bank.instruments, Indices(bank.instruments.size()))
                        | std::views::transform([](auto const instrument_n) { return InstrumentJson(std::get<0>(instrument_n), std::get<1>(instrument_n)); }) | std::ranges::to<std::vector>();
  json["drum_kits"]   = std::views::zip(bank.drum_kits, Indices(bank.drum_kits.size())) | std::views::transform([](auto const kit_n) { return DrumKitJson(std::get<0>(kit_n), std::get<1>(kit_n)); })
                        | std::ranges::to<std::vector>();
  json["tracks"]      = bank.tracks | std::views::transform(TrackJson) | std::ranges::to<std::vector>();
  auto table          = Ordered::object();
  table["format"]     = TABLE_FORMAT;
  table[MUSIC_BANK]   = std::move(json);
  return table.dump();
}

auto ReadMusicBank(Bytes const bytes) -> Packed<MusicBank> {
  return ReadTable(bytes, MUSIC_BANK, MusicBankOf);
}

}
