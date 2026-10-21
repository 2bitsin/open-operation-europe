#pragma once
// The 26 data files of the 1994 English DOS release that the game and the opening read: each one's name, size and
// fingerprint (FNV-1a 64 of its bytes, fingerprint.hpp). Facts about the files, not their content: a player's data
// directory is checked against them before the game starts.

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace opeu::assets::detail::data_files {
using namespace std::string_view_literals;

struct DataFile {
  std::string_view name;        // as shipped, upper case
  std::size_t      size;        // in bytes
  std::uint64_t    fingerprint; // FNV-1a 64 of the bytes

  auto operator==(DataFile const&) const -> bool = default;
};

// The release the files must come from, as a player knows it.
inline constexpr auto RELEASE{ "Operation Europe: Path to Victory 1939-45 (KOEI, 1994), the English DOS release"sv };

// clang-format off
inline constexpr auto DATA_FILES{ std::to_array<DataFile>({
  { .name = "ARMS.SIL"sv,     .size = 121140, .fingerprint = 0x8C75FD74668AAED3 },
  { .name = "BIGMAP.DAT"sv,   .size = 18432,  .fingerprint = 0x8EDA879DFC142480 },
  { .name = "EMSG"sv,         .size = 6104,   .fingerprint = 0xFCBCCECFFD09FD12 },
  { .name = "EMSG.16P"sv,     .size = 6104,   .fingerprint = 0xFCBCCECFFD09FD12 },
  { .name = "ENDGRP.NPK"sv,   .size = 252434, .fingerprint = 0xCC585137F174791C },
  { .name = "EVENT.DAT"sv,    .size = 261120, .fingerprint = 0x5A92C8D09A236933 },
  { .name = "FACE.DAT"sv,     .size = 230400, .fingerprint = 0x1193727F368304BA },
  { .name = "HEXDISP.DAT"sv,  .size = 9600,   .fingerprint = 0xA45525DBAC7A119B },
  { .name = "HEXFONT.DAT"sv,  .size = 61750,  .fingerprint = 0xC14C730AE3CBCD26 },
  { .name = "HEXFONTS.DAT"sv, .size = 61750,  .fingerprint = 0x014205A0A57AC7F4 },
  { .name = "HEXMAP.DAT"sv,   .size = 105252, .fingerprint = 0x0A3115F5326CD508 },
  { .name = "HEXUNIT.DAT"sv,  .size = 34400,  .fingerprint = 0xA527FF6283C952CC },
  { .name = "KOEILOGO.FNT"sv, .size = 8465,   .fingerprint = 0x76DA35AFD40B91D0 },
  { .name = "MAPINF.DAT"sv,   .size = 9600,   .fingerprint = 0xD74D07CA32118223 },
  { .name = "MAPPNO.DAT"sv,   .size = 9600,   .fingerprint = 0xA44B215500D90E6D },
  { .name = "MENDGRP.DAT"sv,  .size = 16384,  .fingerprint = 0x4FB095D497247784 },
  { .name = "MGAMEN.DAT"sv,   .size = 21504,  .fingerprint = 0x779F17564C7749F5 },
  { .name = "MIDMAP.DAT"sv,   .size = 165888, .fingerprint = 0x7C13225AE7065B6F },
  { .name = "MOPENGRP.DAT"sv, .size = 27968,  .fingerprint = 0xD4411DE2AD0316D4 },
  { .name = "MSG"sv,          .size = 30240,  .fingerprint = 0x332800C7910004DB },
  { .name = "MSG.16P"sv,      .size = 30240,  .fingerprint = 0x332800C7910004DB },
  { .name = "MSGRP.DAT"sv,    .size = 23296,  .fingerprint = 0x7E452B44678FD5A3 },
  { .name = "OPENGRP.NPK"sv,  .size = 408291, .fingerprint = 0xBDD92A4C146D70D2 },
  { .name = "SCEDATA"sv,      .size = 66601,  .fingerprint = 0xE3C192126267934F },
  { .name = "SCEEND.DAT"sv,   .size = 140800, .fingerprint = 0xDA9B2BB31D295A45 },
  { .name = "SCESEL.DAT"sv,   .size = 53760,  .fingerprint = 0xF63819FE94CB9188 },
}) };
// clang-format on

}

namespace opeu::assets {
using detail::data_files::DATA_FILES;
using detail::data_files::DataFile;
using detail::data_files::RELEASE;
}
