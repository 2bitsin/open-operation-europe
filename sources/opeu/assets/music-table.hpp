#pragma once
// The FM banks as JSON (music/*.json beside the classic pack, docs/asset-pack.md "Music"): each bank's entries by the
// tracks they play, its instruments and drum kits with their registers by field, and its tracks as named events; read
// back into the MusicBank whose encoding is the bank's bytes.

#include "opeu/assets/bytes.hpp"
#include "opeu/assets/music-bank.hpp"
#include "opeu/assets/pack-error.hpp"

#include <array>
#include <string>
#include <string_view>

namespace opeu::assets::detail::music_table {

// A bank's table file and the layout of its bank.
struct MusicTable {
  std::string_view path;
  BankLayout       layout;
};

inline constexpr auto MUSIC_TABLES{
  std::to_array<MusicTable>({
                             { .path = "music/main-songs.json", .layout = music_bank::MAIN_SONGS },
                             { .path = "music/main-effects.json", .layout = music_bank::MAIN_EFFECTS },
                             { .path = "music/opening-songs.json", .layout = music_bank::OPENING_SONGS },
                             }
  )
};

[[nodiscard]] auto MusicBankText(MusicBank const& bank) -> std::string;

[[nodiscard]] auto ReadMusicBank(Bytes bytes) -> Packed<MusicBank>;

}

namespace opeu::assets {
using detail::music_table::MUSIC_TABLES;
using detail::music_table::MusicBankText;
using detail::music_table::MusicTable;
using detail::music_table::ReadMusicBank;
}
