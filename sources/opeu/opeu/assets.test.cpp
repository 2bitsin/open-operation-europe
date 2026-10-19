// The embedded table answers every name the game and the opening open.
#include "assets.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>

namespace opeu::app::detail::assets_test {

auto Size(std::string_view const name) -> std::size_t {
  auto const found{ AssetFind(name) };
  return found ? found->size() : 0;
}

TEST(Assets, HoldEveryShippedDataFile) {
  constexpr auto NAMES{
    std::array{ "ARMS.SIL",     "BIGMAP.DAT", "EMSG",        "EMSG.16P",     "ENDGRP.NPK",  "EVENT.DAT",  "FACE.DAT",    "HEXDISP.DAT", "HEXFONT.DAT",
               "HEXFONTS.DAT", "HEXMAP.DAT", "HEXUNIT.DAT", "KOEILOGO.FNT", "MAPINF.DAT",  "MAPPNO.DAT", "MENDGRP.DAT", "MGAMEN.DAT",  "MIDMAP.DAT",
               "MOPENGRP.DAT", "MSG",        "MSG.16P",     "MSGRP.DAT",    "OPENGRP.NPK", "SCEDATA",    "SCEEND.DAT",  "SCESEL.DAT" }
  };
  for (std::string_view const name : NAMES) {
    EXPECT_GT(Size(name), 0U) << name;
  }
}

TEST(Assets, FindDosNamesInAnyCaseAndDirectory) {
  EXPECT_EQ(Size("scedata"), Size("SCEDATA"));
  EXPECT_EQ(Size("C:\\OPEU\\Msg.16p"), Size("MSG.16P"));
  EXPECT_EQ(Size("data/face.dat"), 230400U);
}

TEST(Assets, RefuseWhatWasNotShipped) {
  EXPECT_EQ(Size("SAVEDATA"), 0U);
  EXPECT_EQ(Size("MAIN.EXE"), 0U);
  EXPECT_EQ(Size(""), 0U);
}

}
