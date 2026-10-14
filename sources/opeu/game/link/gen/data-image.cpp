// The far data objects of the data modules (game/data) at their segment:offset. Only their addresses, sizes and bytes
// are taken here; their types and contents stay in the data headers. The data segment's own fields are written as typed
// fields instead (data/image.cpp); its map state starts zeroed with the segment.
#include "link/image-block.hpp"

#include "data/fadvice.hpp"
#include "data/fbattle.hpp"
#include "data/fg349b.hpp"
#include "data/fg352d.hpp"
#include "data/fg3546.hpp"
#include "data/fg35c0.hpp"
#include "data/fg35e4.hpp"
#include "data/fg361c.hpp"
#include "data/fg3873.hpp"
#include "data/fg3885.hpp"
#include "data/fg3a64.hpp"
#include "data/fg3a99.hpp"
#include "data/fg3ab9.hpp"
#include "data/fg4582.hpp"
#include "data/fmusic.hpp"
#include "data/fnews.hpp"
#include "data/freply.hpp"

#include <tuple>
#include <vector>

namespace opeu::game::detail::image_block {
namespace {
// clang-format off
static_assert(sizeof(VIEW_CELL_TILES) == 288);
static_assert(sizeof(TURN_FRAME_SHORT_TILES) == 576);
static_assert(sizeof(TURN_FRAME_TALL_TILES) == 768);
static_assert(sizeof(ELEMENT_TYPE_ICONS) == 704);
static_assert(sizeof(BATTLE_HEX_MASKS) == 390);
static_assert(sizeof(YES_NO_POINTER_SAVE) == 960);
static_assert(sizeof(TEXT_CURSOR_TILE) == 32);
static_assert(sizeof(YES_NO_POINTER_IMAGE) == 768);
static_assert(sizeof(YES_NO_POINTER_SHAPE) == 192);
static_assert(sizeof(MENU_BUTTON_TILES) == 576);
static_assert(sizeof(SETUP_MARKER_SAVE) == 512);
static_assert(sizeof(SETUP_MARKER_IMAGE) == 384);
static_assert(sizeof(OVERVIEW_TYPE_MASKS) == 80);
static_assert(sizeof(OVERVIEW_TYPE_SPRITES) == 240);
static_assert(sizeof(OVERVIEW_ORDER_MASKS) == 72);
static_assert(sizeof(OVERVIEW_ORDER_SPRITES) == 216);
static_assert(sizeof(OVERVIEW_BLAST_FRAMES) == 48);
static_assert(sizeof(VIEW_TYPE_SPRITES) == 864);
static_assert(sizeof(VIEW_TYPE_MASKS) == 2592);
static_assert(sizeof(VIEW_SHADOW_IMAGES) == 144);
static_assert(sizeof(VIEW_ORDER_SPRITES) == 1944);
static_assert(sizeof(VIEW_ORDER_MASKS) == 1944);
static_assert(sizeof(VIEW_BLAST_SPRITES) == 216);
static_assert(sizeof(VIEW_BLAST_MASKS) == 648);
static_assert(sizeof(BATTLE_REPORT_0) == 73);
static_assert(sizeof(BATTLE_REPORT_1) == 125);
static_assert(sizeof(BATTLE_REPORT_2) == 125);
static_assert(sizeof(BATTLE_REPORT_3) == 121);
static_assert(sizeof(BATTLE_REPORT_4) == 46);
static_assert(sizeof(BATTLE_REPORT_5) == 48);
static_assert(sizeof(BATTLE_REPORT_6) == 38);
static_assert(sizeof(CONTEXT_FRAME_TILES) == 288);
static_assert(sizeof(LIST_ARROW_TILES) == 256);
static_assert(sizeof(WAR_NEWS_000) == 46);
static_assert(sizeof(WAR_NEWS_001) == 47);
static_assert(sizeof(WAR_NEWS_002) == 59);
static_assert(sizeof(WAR_NEWS_003) == 56);
static_assert(sizeof(WAR_NEWS_004) == 59);
static_assert(sizeof(WAR_NEWS_005) == 61);
static_assert(sizeof(WAR_NEWS_006) == 46);
static_assert(sizeof(WAR_NEWS_007) == 66);
static_assert(sizeof(WAR_NEWS_008) == 48);
static_assert(sizeof(WAR_NEWS_009) == 66);
static_assert(sizeof(WAR_NEWS_010) == 60);
static_assert(sizeof(WAR_NEWS_011) == 54);
static_assert(sizeof(WAR_NEWS_012) == 41);
static_assert(sizeof(WAR_NEWS_013) == 38);
static_assert(sizeof(WAR_NEWS_014) == 61);
static_assert(sizeof(WAR_NEWS_015) == 68);
static_assert(sizeof(WAR_NEWS_016) == 49);
static_assert(sizeof(WAR_NEWS_017) == 61);
static_assert(sizeof(WAR_NEWS_018) == 53);
static_assert(sizeof(WAR_NEWS_019) == 60);
static_assert(sizeof(WAR_NEWS_020) == 31);
static_assert(sizeof(WAR_NEWS_021) == 62);
static_assert(sizeof(WAR_NEWS_022) == 49);
static_assert(sizeof(WAR_NEWS_023) == 62);
static_assert(sizeof(WAR_NEWS_024) == 68);
static_assert(sizeof(WAR_NEWS_025) == 43);
static_assert(sizeof(WAR_NEWS_026) == 35);
static_assert(sizeof(WAR_NEWS_027) == 44);
static_assert(sizeof(WAR_NEWS_028) == 69);
static_assert(sizeof(WAR_NEWS_029) == 69);
static_assert(sizeof(WAR_NEWS_030) == 66);
static_assert(sizeof(WAR_NEWS_031) == 63);
static_assert(sizeof(WAR_NEWS_032) == 38);
static_assert(sizeof(WAR_NEWS_033) == 42);
static_assert(sizeof(WAR_NEWS_034) == 74);
static_assert(sizeof(WAR_NEWS_035) == 77);
static_assert(sizeof(WAR_NEWS_036) == 69);
static_assert(sizeof(WAR_NEWS_037) == 26);
static_assert(sizeof(WAR_NEWS_038) == 77);
static_assert(sizeof(WAR_NEWS_039) == 77);
static_assert(sizeof(WAR_NEWS_040) == 64);
static_assert(sizeof(WAR_NEWS_041) == 45);
static_assert(sizeof(WAR_NEWS_042) == 40);
static_assert(sizeof(WAR_NEWS_043) == 43);
static_assert(sizeof(WAR_NEWS_044) == 59);
static_assert(sizeof(WAR_NEWS_045) == 27);
static_assert(sizeof(WAR_NEWS_046) == 70);
static_assert(sizeof(WAR_NEWS_047) == 53);
static_assert(sizeof(WAR_NEWS_048) == 82);
static_assert(sizeof(WAR_NEWS_049) == 61);
static_assert(sizeof(WAR_NEWS_050) == 55);
static_assert(sizeof(WAR_NEWS_051) == 38);
static_assert(sizeof(WAR_NEWS_052) == 48);
static_assert(sizeof(WAR_NEWS_053) == 31);
static_assert(sizeof(WAR_NEWS_054) == 59);
static_assert(sizeof(WAR_NEWS_055) == 42);
static_assert(sizeof(WAR_NEWS_056) == 62);
static_assert(sizeof(WAR_NEWS_057) == 60);
static_assert(sizeof(WAR_NEWS_058) == 66);
static_assert(sizeof(WAR_NEWS_059) == 76);
static_assert(sizeof(WAR_NEWS_060) == 48);
static_assert(sizeof(WAR_NEWS_061) == 44);
static_assert(sizeof(WAR_NEWS_062) == 51);
static_assert(sizeof(WAR_NEWS_063) == 74);
static_assert(sizeof(WAR_NEWS_064) == 44);
static_assert(sizeof(WAR_NEWS_065) == 85);
static_assert(sizeof(WAR_NEWS_066) == 60);
static_assert(sizeof(WAR_NEWS_067) == 42);
static_assert(sizeof(WAR_NEWS_068) == 55);
static_assert(sizeof(WAR_NEWS_069) == 37);
static_assert(sizeof(WAR_NEWS_070) == 58);
static_assert(sizeof(WAR_NEWS_071) == 41);
static_assert(sizeof(WAR_NEWS_072) == 42);
static_assert(sizeof(WAR_NEWS_073) == 60);
static_assert(sizeof(WAR_NEWS_074) == 40);
static_assert(sizeof(WAR_NEWS_075) == 54);
static_assert(sizeof(WAR_NEWS_076) == 52);
static_assert(sizeof(WAR_NEWS_077) == 32);
static_assert(sizeof(WAR_NEWS_078) == 77);
static_assert(sizeof(WAR_NEWS_079) == 37);
static_assert(sizeof(WAR_NEWS_080) == 58);
static_assert(sizeof(WAR_NEWS_081) == 68);
static_assert(sizeof(WAR_NEWS_082) == 78);
static_assert(sizeof(WAR_NEWS_083) == 52);
static_assert(sizeof(WAR_NEWS_084) == 37);
static_assert(sizeof(WAR_NEWS_085) == 79);
static_assert(sizeof(WAR_NEWS_086) == 79);
static_assert(sizeof(WAR_NEWS_087) == 37);
static_assert(sizeof(WAR_NEWS_088) == 47);
static_assert(sizeof(WAR_NEWS_089) == 53);
static_assert(sizeof(WAR_NEWS_090) == 47);
static_assert(sizeof(WAR_NEWS_091) == 49);
static_assert(sizeof(WAR_NEWS_092) == 41);
static_assert(sizeof(WAR_NEWS_093) == 43);
static_assert(sizeof(WAR_NEWS_094) == 28);
static_assert(sizeof(WAR_NEWS_095) == 68);
static_assert(sizeof(WAR_NEWS_096) == 36);
static_assert(sizeof(WAR_NEWS_097) == 56);
static_assert(sizeof(WAR_NEWS_098) == 62);
static_assert(sizeof(WAR_NEWS_099) == 45);
static_assert(sizeof(WAR_NEWS_100) == 40);
static_assert(sizeof(WAR_NEWS_101) == 37);
static_assert(sizeof(WAR_NEWS_102) == 40);
static_assert(sizeof(WAR_NEWS_103) == 52);
static_assert(sizeof(WAR_NEWS_104) == 81);
static_assert(sizeof(WAR_NEWS_105) == 39);
static_assert(sizeof(WAR_NEWS_106) == 30);
static_assert(sizeof(WAR_NEWS_107) == 39);
static_assert(sizeof(WAR_NEWS_108) == 33);
static_assert(sizeof(WAR_NEWS_109) == 43);
static_assert(sizeof(WAR_NEWS_110) == 51);
static_assert(sizeof(WAR_NEWS_111) == 37);
static_assert(sizeof(WAR_NEWS_112) == 40);
static_assert(sizeof(WAR_NEWS_113) == 46);
static_assert(sizeof(WAR_NEWS_114) == 65);
static_assert(sizeof(WAR_NEWS_115) == 73);
static_assert(sizeof(WAR_NEWS_116) == 57);
static_assert(sizeof(WAR_NEWS_117) == 41);
static_assert(sizeof(WAR_NEWS_118) == 33);
static_assert(sizeof(WAR_NEWS_119) == 64);
static_assert(sizeof(WAR_NEWS_120) == 63);
static_assert(sizeof(WAR_NEWS_121) == 80);
static_assert(sizeof(WAR_NEWS_122) == 63);
static_assert(sizeof(WAR_NEWS_123) == 46);
static_assert(sizeof(WAR_NEWS_124) == 37);
static_assert(sizeof(WAR_NEWS_125) == 51);
static_assert(sizeof(WAR_NEWS_126) == 65);
static_assert(sizeof(WAR_NEWS_127) == 44);
static_assert(sizeof(WAR_NEWS_128) == 50);
static_assert(sizeof(WAR_NEWS_129) == 53);
static_assert(sizeof(WAR_NEWS_130) == 82);
static_assert(sizeof(WAR_NEWS_131) == 37);
static_assert(sizeof(WAR_NEWS_132) == 36);
static_assert(sizeof(WAR_NEWS_133) == 61);
static_assert(sizeof(WAR_NEWS_134) == 29);
static_assert(sizeof(WAR_NEWS_135) == 35);
static_assert(sizeof(WAR_NEWS_136) == 28);
static_assert(sizeof(WAR_NEWS_137) == 27);
static_assert(sizeof(WAR_NEWS_138) == 39);
static_assert(sizeof(WAR_NEWS_139) == 43);
static_assert(sizeof(WAR_NEWS_140) == 30);
static_assert(sizeof(WAR_NEWS_141) == 49);
static_assert(sizeof(WEATHER_TERRAIN_RATINGS) == 330);
static_assert(sizeof(AIR_SPRITE_SAVE) == 518);
static_assert(sizeof(BOMB_SPRITE_SAVE) == 512);
static_assert(sizeof(PANEL_BORDER_TILES) == 512);
static_assert(sizeof(ADVICE_00) == 37);
static_assert(sizeof(ADVICE_01) == 33);
static_assert(sizeof(ADVICE_02) == 75);
static_assert(sizeof(ADVICE_03) == 67);
static_assert(sizeof(ADVICE_04) == 51);
static_assert(sizeof(ADVICE_05) == 40);
static_assert(sizeof(ADVICE_06) == 29);
static_assert(sizeof(ADVICE_07) == 35);
static_assert(sizeof(ADVICE_08) == 29);
static_assert(sizeof(ADVICE_09) == 59);
static_assert(sizeof(ADVICE_10) == 73);
static_assert(sizeof(ADVICE_11) == 68);
static_assert(sizeof(ADVICE_12) == 36);
static_assert(sizeof(ADVICE_13) == 50);
static_assert(sizeof(ADVICE_14) == 60);
static_assert(sizeof(ADVICE_15) == 49);
static_assert(sizeof(ADVICE_16) == 44);
static_assert(sizeof(ADVICE_17) == 39);
static_assert(sizeof(ADVICE_18) == 44);
static_assert(sizeof(ADVICE_19) == 21);
static_assert(sizeof(ADVICE_20) == 51);
static_assert(sizeof(ADVICE_21) == 55);
static_assert(sizeof(ADVICE_22) == 40);
static_assert(sizeof(ADVICE_23) == 38);
static_assert(sizeof(ADVICE_24) == 40);
static_assert(sizeof(ADVICE_25) == 19);
static_assert(sizeof(ADVICE_26) == 38);
static_assert(sizeof(ADVICE_27) == 33);
static_assert(sizeof(ADVICE_28) == 45);
static_assert(sizeof(ADVICE_29) == 30);
static_assert(sizeof(ADVICE_30) == 50);
static_assert(sizeof(ADVICE_31) == 48);
static_assert(sizeof(ADVICE_32) == 61);
static_assert(sizeof(ADVICE_33) == 33);
static_assert(sizeof(ADVICE_34) == 47);
static_assert(sizeof(ADVICE_35) == 60);
static_assert(sizeof(ADVICE_36) == 67);
static_assert(sizeof(ADVICE_37) == 41);
static_assert(sizeof(ADVICE_38) == 27);
static_assert(sizeof(ADVICE_39) == 36);
static_assert(sizeof(ADVICE_40) == 49);
static_assert(sizeof(ADVICE_41) == 55);
static_assert(sizeof(ADVICE_42) == 51);
static_assert(sizeof(ADVICE_43) == 39);
static_assert(sizeof(ADVICE_44) == 46);
static_assert(sizeof(ADVICE_45) == 44);
static_assert(sizeof(ADVICE_46) == 49);
static_assert(sizeof(ADVICE_47) == 71);
static_assert(sizeof(ADVICE_48) == 52);
static_assert(sizeof(ADVICE_49) == 39);
static_assert(sizeof(ADVICE_50) == 58);
static_assert(sizeof(ADVICE_51) == 49);
static_assert(sizeof(ADVICE_52) == 37);
static_assert(sizeof(ADVICE_53) == 51);
static_assert(sizeof(ADVICE_54) == 36);
static_assert(sizeof(ADVICE_55) == 66);
static_assert(sizeof(ADVICE_56) == 35);
static_assert(sizeof(ADVICE_57) == 33);
static_assert(sizeof(ADVICE_58) == 30);
static_assert(sizeof(ADVICE_59) == 41);
static_assert(sizeof(ADVICE_60) == 29);
static_assert(sizeof(ADVICE_61) == 21);
static_assert(sizeof(ADVICE_62) == 31);
static_assert(sizeof(ADVICE_63) == 76);
static_assert(sizeof(ADVICE_64) == 51);
static_assert(sizeof(ADVICE_65) == 70);
static_assert(sizeof(ADVICE_66) == 53);
static_assert(sizeof(ADVICE_67) == 79);
static_assert(sizeof(ADVICE_68) == 71);
static_assert(sizeof(ADVICE_69) == 40);
static_assert(sizeof(ADVICE_70) == 41);
static_assert(sizeof(ADVICE_71) == 55);
static_assert(sizeof(ADVICE_72) == 86);
static_assert(sizeof(ADVICE_73) == 69);
static_assert(sizeof(ADVICE_74) == 89);
static_assert(sizeof(ADVICE_75) == 50);
static_assert(sizeof(ADVICE_76) == 85);
static_assert(sizeof(ADVICE_77) == 54);
static_assert(sizeof(ADVICE_78) == 66);
static_assert(sizeof(ADVICE_79) == 72);
static_assert(sizeof(ADVICE_80) == 45);
static_assert(sizeof(ADVICE_81) == 88);
static_assert(sizeof(ADVICE_82) == 73);
static_assert(sizeof(ADVICE_83) == 55);
static_assert(sizeof(ADVICE_84) == 77);
static_assert(sizeof(ADVICE_85) == 60);
static_assert(sizeof(ADVICE_86) == 87);
static_assert(sizeof(ADVICE_87) == 53);
static_assert(sizeof(ADVICE_88) == 76);
static_assert(sizeof(ADVICE_89) == 67);
static_assert(sizeof(ADVICE_90) == 67);
static_assert(sizeof(ADVICE_91) == 64);
static_assert(sizeof(ADVICE_92) == 69);
static_assert(sizeof(REPLY_000) == 54);
static_assert(sizeof(REPLY_001) == 33);
static_assert(sizeof(REPLY_002) == 38);
static_assert(sizeof(REPLY_003) == 26);
static_assert(sizeof(REPLY_004) == 56);
static_assert(sizeof(REPLY_005) == 75);
static_assert(sizeof(REPLY_006) == 23);
static_assert(sizeof(REPLY_007) == 53);
static_assert(sizeof(REPLY_008) == 53);
static_assert(sizeof(REPLY_009) == 49);
static_assert(sizeof(REPLY_010) == 54);
static_assert(sizeof(REPLY_011) == 39);
static_assert(sizeof(REPLY_012) == 53);
static_assert(sizeof(REPLY_013) == 75);
static_assert(sizeof(REPLY_014) == 61);
static_assert(sizeof(REPLY_015) == 73);
static_assert(sizeof(REPLY_016) == 46);
static_assert(sizeof(REPLY_017) == 43);
static_assert(sizeof(REPLY_018) == 51);
static_assert(sizeof(REPLY_019) == 27);
static_assert(sizeof(REPLY_020) == 71);
static_assert(sizeof(REPLY_021) == 1);
static_assert(sizeof(REPLY_022) == 54);
static_assert(sizeof(REPLY_023) == 43);
static_assert(sizeof(REPLY_024) == 59);
static_assert(sizeof(REPLY_025) == 49);
static_assert(sizeof(REPLY_026) == 73);
static_assert(sizeof(REPLY_027) == 36);
static_assert(sizeof(REPLY_028) == 37);
static_assert(sizeof(REPLY_029) == 37);
static_assert(sizeof(REPLY_030) == 36);
static_assert(sizeof(REPLY_031) == 41);
static_assert(sizeof(REPLY_032) == 50);
static_assert(sizeof(REPLY_033) == 24);
static_assert(sizeof(REPLY_034) == 32);
static_assert(sizeof(REPLY_035) == 32);
static_assert(sizeof(REPLY_036) == 51);
static_assert(sizeof(REPLY_037) == 57);
static_assert(sizeof(REPLY_038) == 49);
static_assert(sizeof(REPLY_039) == 45);
static_assert(sizeof(REPLY_040) == 67);
static_assert(sizeof(REPLY_041) == 37);
static_assert(sizeof(REPLY_042) == 50);
static_assert(sizeof(REPLY_043) == 25);
static_assert(sizeof(REPLY_044) == 44);
static_assert(sizeof(REPLY_045) == 75);
static_assert(sizeof(REPLY_046) == 72);
static_assert(sizeof(REPLY_047) == 56);
static_assert(sizeof(REPLY_048) == 55);
static_assert(sizeof(REPLY_049) == 24);
static_assert(sizeof(REPLY_050) == 84);
static_assert(sizeof(REPLY_051) == 50);
static_assert(sizeof(REPLY_052) == 54);
static_assert(sizeof(REPLY_053) == 59);
static_assert(sizeof(REPLY_054) == 46);
static_assert(sizeof(REPLY_055) == 54);
static_assert(sizeof(REPLY_056) == 83);
static_assert(sizeof(REPLY_057) == 18);
static_assert(sizeof(REPLY_058) == 59);
static_assert(sizeof(REPLY_059) == 28);
static_assert(sizeof(REPLY_060) == 21);
static_assert(sizeof(REPLY_061) == 54);
static_assert(sizeof(REPLY_062) == 53);
static_assert(sizeof(REPLY_063) == 60);
static_assert(sizeof(REPLY_064) == 65);
static_assert(sizeof(REPLY_065) == 61);
static_assert(sizeof(REPLY_066) == 70);
static_assert(sizeof(REPLY_067) == 62);
static_assert(sizeof(REPLY_068) == 46);
static_assert(sizeof(REPLY_069) == 44);
static_assert(sizeof(REPLY_070) == 72);
static_assert(sizeof(REPLY_071) == 67);
static_assert(sizeof(REPLY_072) == 33);
static_assert(sizeof(REPLY_073) == 41);
static_assert(sizeof(REPLY_074) == 48);
static_assert(sizeof(REPLY_075) == 36);
static_assert(sizeof(REPLY_076) == 58);
static_assert(sizeof(REPLY_077) == 53);
static_assert(sizeof(REPLY_078) == 53);
static_assert(sizeof(REPLY_079) == 54);
static_assert(sizeof(REPLY_080) == 70);
static_assert(sizeof(REPLY_081) == 86);
static_assert(sizeof(REPLY_082) == 54);
static_assert(sizeof(REPLY_083) == 56);
static_assert(sizeof(REPLY_084) == 42);
static_assert(sizeof(REPLY_085) == 85);
static_assert(sizeof(REPLY_086) == 82);
static_assert(sizeof(REPLY_087) == 49);
static_assert(sizeof(REPLY_088) == 69);
static_assert(sizeof(REPLY_089) == 83);
static_assert(sizeof(REPLY_090) == 66);
static_assert(sizeof(REPLY_091) == 82);
static_assert(sizeof(REPLY_092) == 69);
static_assert(sizeof(REPLY_093) == 79);
static_assert(sizeof(REPLY_094) == 54);
static_assert(sizeof(REPLY_095) == 45);
static_assert(sizeof(REPLY_096) == 31);
static_assert(sizeof(REPLY_097) == 43);
static_assert(sizeof(REPLY_098) == 66);
static_assert(sizeof(REPLY_099) == 19);
static_assert(sizeof(REPLY_100) == 48);
static_assert(sizeof(REPLY_101) == 31);
static_assert(sizeof(REPLY_102) == 33);
static_assert(sizeof(REPLY_103) == 28);
static_assert(sizeof(REPLY_104) == 40);
static_assert(sizeof(REPLY_105) == 58);
static_assert(sizeof(REPLY_106) == 24);
static_assert(sizeof(REPLY_107) == 44);
static_assert(sizeof(REPLY_108) == 76);
static_assert(sizeof(REPLY_109) == 36);
static_assert(sizeof(REPLY_110) == 41);
static_assert(sizeof(REPLY_111) == 46);
static_assert(sizeof(REPLY_112) == 43);
static_assert(sizeof(REPLY_113) == 41);
static_assert(sizeof(REPLY_114) == 44);
static_assert(sizeof(REPLY_115) == 75);
static_assert(sizeof(REPLY_116) == 54);
static_assert(sizeof(REPLY_117) == 35);
static_assert(sizeof(REPLY_118) == 29);
static_assert(sizeof(REPLY_119) == 41);
static_assert(sizeof(REPLY_120) == 73);
static_assert(sizeof(REPLY_121) == 47);
static_assert(sizeof(REPLY_122) == 63);
static_assert(sizeof(REPLY_123) == 52);
static_assert(sizeof(REPLY_124) == 48);
static_assert(sizeof(MUSIC_TRACKS) == 190);
static_assert(sizeof(MUSIC_VOICES) == 1629);
static_assert(sizeof(MUSIC_EVENTS) == 29855);
static_assert(sizeof(SOUND_EFFECTS) == 847);
static_assert(sizeof(PATH_CELL_QUEUE) == 3200);
static_assert(sizeof(PATH_COST_GRID) == 3200);

constexpr std::tuple FAR_OBJECTS{
  FarObject{ { 0x349b, 0x0000 }, VIEW_CELL_TILES         }, // data/fg349b
  FarObject{ { 0x349b, 0x0120 }, TURN_FRAME_SHORT_TILES  }, // data/fg349b
  FarObject{ { 0x349b, 0x0360 }, TURN_FRAME_TALL_TILES   }, // data/fg349b
  FarObject{ { 0x349b, 0x0660 }, ELEMENT_TYPE_ICONS      }, // data/fg349b
  FarObject{ { 0x352d, 0x0000 }, BATTLE_HEX_MASKS        }, // data/fg352d
  FarObject{ { 0x3546, 0x0000 }, YES_NO_POINTER_SAVE     }, // data/fg3546
  FarObject{ { 0x3546, 0x03c0 }, TEXT_CURSOR_TILE        }, // data/fg3546
  FarObject{ { 0x3546, 0x03e0 }, YES_NO_POINTER_IMAGE    }, // data/fg3546
  FarObject{ { 0x3546, 0x06e0 }, YES_NO_POINTER_SHAPE    }, // data/fg3546
  FarObject{ { 0x35c0, 0x0000 }, MENU_BUTTON_TILES       }, // data/fg35c0
  FarObject{ { 0x35e4, 0x0000 }, SETUP_MARKER_SAVE       }, // data/fg35e4
  FarObject{ { 0x35e4, 0x0200 }, SETUP_MARKER_IMAGE      }, // data/fg35e4
  FarObject{ { 0x361c, 0x0000 }, OVERVIEW_TYPE_MASKS     }, // data/fg361c
  FarObject{ { 0x361c, 0x0050 }, OVERVIEW_TYPE_SPRITES   }, // data/fg361c
  FarObject{ { 0x361c, 0x0140 }, OVERVIEW_ORDER_MASKS    }, // data/fg361c
  FarObject{ { 0x361c, 0x0188 }, OVERVIEW_ORDER_SPRITES  }, // data/fg361c
  FarObject{ { 0x361c, 0x0260 }, OVERVIEW_BLAST_FRAMES   }, // data/fg361c
  FarObject{ { 0x361c, 0x0290 }, VIEW_TYPE_SPRITES       }, // data/fg361c
  FarObject{ { 0x361c, 0x05f0 }, VIEW_TYPE_MASKS         }, // data/fg361c
  FarObject{ { 0x361c, 0x1010 }, VIEW_SHADOW_IMAGES      }, // data/fg361c
  FarObject{ { 0x361c, 0x10a0 }, VIEW_ORDER_SPRITES      }, // data/fg361c
  FarObject{ { 0x361c, 0x1838 }, VIEW_ORDER_MASKS        }, // data/fg361c
  FarObject{ { 0x361c, 0x1fd0 }, VIEW_BLAST_SPRITES      }, // data/fg361c
  FarObject{ { 0x361c, 0x20a8 }, VIEW_BLAST_MASKS        }, // data/fg361c
  FarObject{ { 0x384f, 0x0000 }, BATTLE_REPORT_0         }, // data/fbattle
  FarObject{ { 0x384f, 0x0049 }, BATTLE_REPORT_1         }, // data/fbattle
  FarObject{ { 0x384f, 0x00c6 }, BATTLE_REPORT_2         }, // data/fbattle
  FarObject{ { 0x384f, 0x0143 }, BATTLE_REPORT_3         }, // data/fbattle
  FarObject{ { 0x384f, 0x01bc }, BATTLE_REPORT_4         }, // data/fbattle
  FarObject{ { 0x384f, 0x01ea }, BATTLE_REPORT_5         }, // data/fbattle
  FarObject{ { 0x384f, 0x021a }, BATTLE_REPORT_6         }, // data/fbattle
  FarObject{ { 0x3873, 0x0000 }, CONTEXT_FRAME_TILES     }, // data/fg3873
  FarObject{ { 0x3885, 0x0000 }, LIST_ARROW_TILES        }, // data/fg3885
  FarObject{ { 0x3895, 0x0000 }, WAR_NEWS_000            }, // data/fnews
  FarObject{ { 0x3895, 0x002e }, WAR_NEWS_001            }, // data/fnews
  FarObject{ { 0x3895, 0x005d }, WAR_NEWS_002            }, // data/fnews
  FarObject{ { 0x3895, 0x0098 }, WAR_NEWS_003            }, // data/fnews
  FarObject{ { 0x3895, 0x00d0 }, WAR_NEWS_004            }, // data/fnews
  FarObject{ { 0x3895, 0x010b }, WAR_NEWS_005            }, // data/fnews
  FarObject{ { 0x3895, 0x0148 }, WAR_NEWS_006            }, // data/fnews
  FarObject{ { 0x3895, 0x0176 }, WAR_NEWS_007            }, // data/fnews
  FarObject{ { 0x3895, 0x01b8 }, WAR_NEWS_008            }, // data/fnews
  FarObject{ { 0x3895, 0x01e8 }, WAR_NEWS_009            }, // data/fnews
  FarObject{ { 0x3895, 0x022a }, WAR_NEWS_010            }, // data/fnews
  FarObject{ { 0x3895, 0x0266 }, WAR_NEWS_011            }, // data/fnews
  FarObject{ { 0x3895, 0x029c }, WAR_NEWS_012            }, // data/fnews
  FarObject{ { 0x3895, 0x02c5 }, WAR_NEWS_013            }, // data/fnews
  FarObject{ { 0x3895, 0x02eb }, WAR_NEWS_014            }, // data/fnews
  FarObject{ { 0x3895, 0x0328 }, WAR_NEWS_015            }, // data/fnews
  FarObject{ { 0x3895, 0x036c }, WAR_NEWS_016            }, // data/fnews
  FarObject{ { 0x3895, 0x039d }, WAR_NEWS_017            }, // data/fnews
  FarObject{ { 0x3895, 0x03da }, WAR_NEWS_018            }, // data/fnews
  FarObject{ { 0x3895, 0x040f }, WAR_NEWS_019            }, // data/fnews
  FarObject{ { 0x3895, 0x044b }, WAR_NEWS_020            }, // data/fnews
  FarObject{ { 0x3895, 0x046a }, WAR_NEWS_021            }, // data/fnews
  FarObject{ { 0x3895, 0x04a8 }, WAR_NEWS_022            }, // data/fnews
  FarObject{ { 0x3895, 0x04d9 }, WAR_NEWS_023            }, // data/fnews
  FarObject{ { 0x3895, 0x0517 }, WAR_NEWS_024            }, // data/fnews
  FarObject{ { 0x3895, 0x055b }, WAR_NEWS_025            }, // data/fnews
  FarObject{ { 0x3895, 0x0586 }, WAR_NEWS_026            }, // data/fnews
  FarObject{ { 0x3895, 0x05a9 }, WAR_NEWS_027            }, // data/fnews
  FarObject{ { 0x3895, 0x05d5 }, WAR_NEWS_028            }, // data/fnews
  FarObject{ { 0x3895, 0x061a }, WAR_NEWS_029            }, // data/fnews
  FarObject{ { 0x3895, 0x065f }, WAR_NEWS_030            }, // data/fnews
  FarObject{ { 0x3895, 0x06a1 }, WAR_NEWS_031            }, // data/fnews
  FarObject{ { 0x3895, 0x06e0 }, WAR_NEWS_032            }, // data/fnews
  FarObject{ { 0x3895, 0x0706 }, WAR_NEWS_033            }, // data/fnews
  FarObject{ { 0x3895, 0x0730 }, WAR_NEWS_034            }, // data/fnews
  FarObject{ { 0x3895, 0x077a }, WAR_NEWS_035            }, // data/fnews
  FarObject{ { 0x3895, 0x07c7 }, WAR_NEWS_036            }, // data/fnews
  FarObject{ { 0x3895, 0x080c }, WAR_NEWS_037            }, // data/fnews
  FarObject{ { 0x3895, 0x0826 }, WAR_NEWS_038            }, // data/fnews
  FarObject{ { 0x3895, 0x0873 }, WAR_NEWS_039            }, // data/fnews
  FarObject{ { 0x3895, 0x08c0 }, WAR_NEWS_040            }, // data/fnews
  FarObject{ { 0x3895, 0x0900 }, WAR_NEWS_041            }, // data/fnews
  FarObject{ { 0x3895, 0x092d }, WAR_NEWS_042            }, // data/fnews
  FarObject{ { 0x3895, 0x0955 }, WAR_NEWS_043            }, // data/fnews
  FarObject{ { 0x3895, 0x0980 }, WAR_NEWS_044            }, // data/fnews
  FarObject{ { 0x3895, 0x09bb }, WAR_NEWS_045            }, // data/fnews
  FarObject{ { 0x3895, 0x09d6 }, WAR_NEWS_046            }, // data/fnews
  FarObject{ { 0x3895, 0x0a1c }, WAR_NEWS_047            }, // data/fnews
  FarObject{ { 0x3895, 0x0a51 }, WAR_NEWS_048            }, // data/fnews
  FarObject{ { 0x3895, 0x0aa3 }, WAR_NEWS_049            }, // data/fnews
  FarObject{ { 0x3895, 0x0ae0 }, WAR_NEWS_050            }, // data/fnews
  FarObject{ { 0x3895, 0x0b17 }, WAR_NEWS_051            }, // data/fnews
  FarObject{ { 0x3895, 0x0b3d }, WAR_NEWS_052            }, // data/fnews
  FarObject{ { 0x3895, 0x0b6d }, WAR_NEWS_053            }, // data/fnews
  FarObject{ { 0x3895, 0x0b8c }, WAR_NEWS_054            }, // data/fnews
  FarObject{ { 0x3895, 0x0bc7 }, WAR_NEWS_055            }, // data/fnews
  FarObject{ { 0x3895, 0x0bf1 }, WAR_NEWS_056            }, // data/fnews
  FarObject{ { 0x3895, 0x0c2f }, WAR_NEWS_057            }, // data/fnews
  FarObject{ { 0x3895, 0x0c6b }, WAR_NEWS_058            }, // data/fnews
  FarObject{ { 0x3895, 0x0cad }, WAR_NEWS_059            }, // data/fnews
  FarObject{ { 0x3895, 0x0cf9 }, WAR_NEWS_060            }, // data/fnews
  FarObject{ { 0x3895, 0x0d29 }, WAR_NEWS_061            }, // data/fnews
  FarObject{ { 0x3895, 0x0d55 }, WAR_NEWS_062            }, // data/fnews
  FarObject{ { 0x3895, 0x0d88 }, WAR_NEWS_063            }, // data/fnews
  FarObject{ { 0x3895, 0x0dd2 }, WAR_NEWS_064            }, // data/fnews
  FarObject{ { 0x3895, 0x0dfe }, WAR_NEWS_065            }, // data/fnews
  FarObject{ { 0x3895, 0x0e53 }, WAR_NEWS_066            }, // data/fnews
  FarObject{ { 0x3895, 0x0e8f }, WAR_NEWS_067            }, // data/fnews
  FarObject{ { 0x3895, 0x0eb9 }, WAR_NEWS_068            }, // data/fnews
  FarObject{ { 0x3895, 0x0ef0 }, WAR_NEWS_069            }, // data/fnews
  FarObject{ { 0x3895, 0x0f15 }, WAR_NEWS_070            }, // data/fnews
  FarObject{ { 0x3895, 0x0f4f }, WAR_NEWS_071            }, // data/fnews
  FarObject{ { 0x3895, 0x0f78 }, WAR_NEWS_072            }, // data/fnews
  FarObject{ { 0x3895, 0x0fa2 }, WAR_NEWS_073            }, // data/fnews
  FarObject{ { 0x3895, 0x0fde }, WAR_NEWS_074            }, // data/fnews
  FarObject{ { 0x3895, 0x1006 }, WAR_NEWS_075            }, // data/fnews
  FarObject{ { 0x3895, 0x103c }, WAR_NEWS_076            }, // data/fnews
  FarObject{ { 0x3895, 0x1070 }, WAR_NEWS_077            }, // data/fnews
  FarObject{ { 0x3895, 0x1090 }, WAR_NEWS_078            }, // data/fnews
  FarObject{ { 0x3895, 0x10dd }, WAR_NEWS_079            }, // data/fnews
  FarObject{ { 0x3895, 0x1102 }, WAR_NEWS_080            }, // data/fnews
  FarObject{ { 0x3895, 0x113c }, WAR_NEWS_081            }, // data/fnews
  FarObject{ { 0x3895, 0x1180 }, WAR_NEWS_082            }, // data/fnews
  FarObject{ { 0x3895, 0x11ce }, WAR_NEWS_083            }, // data/fnews
  FarObject{ { 0x3895, 0x1202 }, WAR_NEWS_084            }, // data/fnews
  FarObject{ { 0x3895, 0x1227 }, WAR_NEWS_085            }, // data/fnews
  FarObject{ { 0x3895, 0x1276 }, WAR_NEWS_086            }, // data/fnews
  FarObject{ { 0x3895, 0x12c5 }, WAR_NEWS_087            }, // data/fnews
  FarObject{ { 0x3895, 0x12ea }, WAR_NEWS_088            }, // data/fnews
  FarObject{ { 0x3895, 0x1319 }, WAR_NEWS_089            }, // data/fnews
  FarObject{ { 0x3895, 0x134e }, WAR_NEWS_090            }, // data/fnews
  FarObject{ { 0x3895, 0x137d }, WAR_NEWS_091            }, // data/fnews
  FarObject{ { 0x3895, 0x13ae }, WAR_NEWS_092            }, // data/fnews
  FarObject{ { 0x3895, 0x13d7 }, WAR_NEWS_093            }, // data/fnews
  FarObject{ { 0x3895, 0x1402 }, WAR_NEWS_094            }, // data/fnews
  FarObject{ { 0x3895, 0x141e }, WAR_NEWS_095            }, // data/fnews
  FarObject{ { 0x3895, 0x1462 }, WAR_NEWS_096            }, // data/fnews
  FarObject{ { 0x3895, 0x1486 }, WAR_NEWS_097            }, // data/fnews
  FarObject{ { 0x3895, 0x14be }, WAR_NEWS_098            }, // data/fnews
  FarObject{ { 0x3895, 0x14fc }, WAR_NEWS_099            }, // data/fnews
  FarObject{ { 0x3895, 0x1529 }, WAR_NEWS_100            }, // data/fnews
  FarObject{ { 0x3895, 0x1551 }, WAR_NEWS_101            }, // data/fnews
  FarObject{ { 0x3895, 0x1576 }, WAR_NEWS_102            }, // data/fnews
  FarObject{ { 0x3895, 0x159e }, WAR_NEWS_103            }, // data/fnews
  FarObject{ { 0x3895, 0x15d2 }, WAR_NEWS_104            }, // data/fnews
  FarObject{ { 0x3895, 0x1623 }, WAR_NEWS_105            }, // data/fnews
  FarObject{ { 0x3895, 0x164a }, WAR_NEWS_106            }, // data/fnews
  FarObject{ { 0x3895, 0x1668 }, WAR_NEWS_107            }, // data/fnews
  FarObject{ { 0x3895, 0x168f }, WAR_NEWS_108            }, // data/fnews
  FarObject{ { 0x3895, 0x16b0 }, WAR_NEWS_109            }, // data/fnews
  FarObject{ { 0x3895, 0x16db }, WAR_NEWS_110            }, // data/fnews
  FarObject{ { 0x3895, 0x170e }, WAR_NEWS_111            }, // data/fnews
  FarObject{ { 0x3895, 0x1733 }, WAR_NEWS_112            }, // data/fnews
  FarObject{ { 0x3895, 0x175b }, WAR_NEWS_113            }, // data/fnews
  FarObject{ { 0x3895, 0x1789 }, WAR_NEWS_114            }, // data/fnews
  FarObject{ { 0x3895, 0x17ca }, WAR_NEWS_115            }, // data/fnews
  FarObject{ { 0x3895, 0x1813 }, WAR_NEWS_116            }, // data/fnews
  FarObject{ { 0x3895, 0x184c }, WAR_NEWS_117            }, // data/fnews
  FarObject{ { 0x3895, 0x1875 }, WAR_NEWS_118            }, // data/fnews
  FarObject{ { 0x3895, 0x1896 }, WAR_NEWS_119            }, // data/fnews
  FarObject{ { 0x3895, 0x18d6 }, WAR_NEWS_120            }, // data/fnews
  FarObject{ { 0x3895, 0x1915 }, WAR_NEWS_121            }, // data/fnews
  FarObject{ { 0x3895, 0x1965 }, WAR_NEWS_122            }, // data/fnews
  FarObject{ { 0x3895, 0x19a4 }, WAR_NEWS_123            }, // data/fnews
  FarObject{ { 0x3895, 0x19d2 }, WAR_NEWS_124            }, // data/fnews
  FarObject{ { 0x3895, 0x19f7 }, WAR_NEWS_125            }, // data/fnews
  FarObject{ { 0x3895, 0x1a2a }, WAR_NEWS_126            }, // data/fnews
  FarObject{ { 0x3895, 0x1a6b }, WAR_NEWS_127            }, // data/fnews
  FarObject{ { 0x3895, 0x1a97 }, WAR_NEWS_128            }, // data/fnews
  FarObject{ { 0x3895, 0x1ac9 }, WAR_NEWS_129            }, // data/fnews
  FarObject{ { 0x3895, 0x1afe }, WAR_NEWS_130            }, // data/fnews
  FarObject{ { 0x3895, 0x1b50 }, WAR_NEWS_131            }, // data/fnews
  FarObject{ { 0x3895, 0x1b75 }, WAR_NEWS_132            }, // data/fnews
  FarObject{ { 0x3895, 0x1b99 }, WAR_NEWS_133            }, // data/fnews
  FarObject{ { 0x3895, 0x1bd6 }, WAR_NEWS_134            }, // data/fnews
  FarObject{ { 0x3895, 0x1bf3 }, WAR_NEWS_135            }, // data/fnews
  FarObject{ { 0x3895, 0x1c16 }, WAR_NEWS_136            }, // data/fnews
  FarObject{ { 0x3895, 0x1c32 }, WAR_NEWS_137            }, // data/fnews
  FarObject{ { 0x3895, 0x1c4d }, WAR_NEWS_138            }, // data/fnews
  FarObject{ { 0x3895, 0x1c74 }, WAR_NEWS_139            }, // data/fnews
  FarObject{ { 0x3895, 0x1c9f }, WAR_NEWS_140            }, // data/fnews
  FarObject{ { 0x3895, 0x1cbd }, WAR_NEWS_141            }, // data/fnews
  FarObject{ { 0x3a64, 0x0000 }, WEATHER_TERRAIN_RATINGS }, // data/fg3a64
  FarObject{ { 0x3a64, 0x014a }, AIR_SPRITE_SAVE         }, // data/fg3a64
  FarObject{ { 0x3a99, 0x0000 }, BOMB_SPRITE_SAVE        }, // data/fg3a99
  FarObject{ { 0x3ab9, 0x0000 }, PANEL_BORDER_TILES      }, // data/fg3ab9
  FarObject{ { 0x3ad9, 0x0000 }, ADVICE_00               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0025 }, ADVICE_01               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0046 }, ADVICE_02               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0091 }, ADVICE_03               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x00d4 }, ADVICE_04               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0107 }, ADVICE_05               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x012f }, ADVICE_06               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x014c }, ADVICE_07               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x016f }, ADVICE_08               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x018c }, ADVICE_09               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x01c7 }, ADVICE_10               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0210 }, ADVICE_11               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0254 }, ADVICE_12               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0278 }, ADVICE_13               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x02aa }, ADVICE_14               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x02e6 }, ADVICE_15               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0317 }, ADVICE_16               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0343 }, ADVICE_17               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x036a }, ADVICE_18               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0396 }, ADVICE_19               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x03ab }, ADVICE_20               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x03de }, ADVICE_21               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0415 }, ADVICE_22               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x043d }, ADVICE_23               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0463 }, ADVICE_24               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x048b }, ADVICE_25               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x049e }, ADVICE_26               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x04c4 }, ADVICE_27               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x04e5 }, ADVICE_28               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0512 }, ADVICE_29               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0530 }, ADVICE_30               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0562 }, ADVICE_31               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0592 }, ADVICE_32               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x05cf }, ADVICE_33               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x05f0 }, ADVICE_34               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x061f }, ADVICE_35               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x065b }, ADVICE_36               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x069e }, ADVICE_37               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x06c7 }, ADVICE_38               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x06e2 }, ADVICE_39               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0706 }, ADVICE_40               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0737 }, ADVICE_41               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x076e }, ADVICE_42               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x07a1 }, ADVICE_43               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x07c8 }, ADVICE_44               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x07f6 }, ADVICE_45               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0822 }, ADVICE_46               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0853 }, ADVICE_47               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x089a }, ADVICE_48               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x08ce }, ADVICE_49               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x08f5 }, ADVICE_50               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x092f }, ADVICE_51               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0960 }, ADVICE_52               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0985 }, ADVICE_53               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x09b8 }, ADVICE_54               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x09dc }, ADVICE_55               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0a1e }, ADVICE_56               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0a41 }, ADVICE_57               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0a62 }, ADVICE_58               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0a80 }, ADVICE_59               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0aa9 }, ADVICE_60               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0ac6 }, ADVICE_61               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0adb }, ADVICE_62               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0afa }, ADVICE_63               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0b46 }, ADVICE_64               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0b79 }, ADVICE_65               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0bbf }, ADVICE_66               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0bf4 }, ADVICE_67               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0c43 }, ADVICE_68               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0c8a }, ADVICE_69               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0cb2 }, ADVICE_70               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0cdb }, ADVICE_71               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0d12 }, ADVICE_72               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0d68 }, ADVICE_73               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0dad }, ADVICE_74               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0e06 }, ADVICE_75               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0e38 }, ADVICE_76               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0e8d }, ADVICE_77               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0ec3 }, ADVICE_78               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0f05 }, ADVICE_79               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0f4d }, ADVICE_80               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0f7a }, ADVICE_81               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x0fd2 }, ADVICE_82               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x101b }, ADVICE_83               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x1052 }, ADVICE_84               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x109f }, ADVICE_85               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x10db }, ADVICE_86               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x1132 }, ADVICE_87               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x1167 }, ADVICE_88               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x11b3 }, ADVICE_89               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x11f6 }, ADVICE_90               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x1239 }, ADVICE_91               }, // data/fadvice
  FarObject{ { 0x3ad9, 0x1279 }, ADVICE_92               }, // data/fadvice
  FarObject{ { 0x3c05, 0x0000 }, REPLY_000               }, // data/freply
  FarObject{ { 0x3c05, 0x0036 }, REPLY_001               }, // data/freply
  FarObject{ { 0x3c05, 0x0057 }, REPLY_002               }, // data/freply
  FarObject{ { 0x3c05, 0x007d }, REPLY_003               }, // data/freply
  FarObject{ { 0x3c05, 0x0097 }, REPLY_004               }, // data/freply
  FarObject{ { 0x3c05, 0x00cf }, REPLY_005               }, // data/freply
  FarObject{ { 0x3c05, 0x011a }, REPLY_006               }, // data/freply
  FarObject{ { 0x3c05, 0x0131 }, REPLY_007               }, // data/freply
  FarObject{ { 0x3c05, 0x0166 }, REPLY_008               }, // data/freply
  FarObject{ { 0x3c05, 0x019b }, REPLY_009               }, // data/freply
  FarObject{ { 0x3c05, 0x01cc }, REPLY_010               }, // data/freply
  FarObject{ { 0x3c05, 0x0202 }, REPLY_011               }, // data/freply
  FarObject{ { 0x3c05, 0x0229 }, REPLY_012               }, // data/freply
  FarObject{ { 0x3c05, 0x025e }, REPLY_013               }, // data/freply
  FarObject{ { 0x3c05, 0x02a9 }, REPLY_014               }, // data/freply
  FarObject{ { 0x3c05, 0x02e6 }, REPLY_015               }, // data/freply
  FarObject{ { 0x3c05, 0x032f }, REPLY_016               }, // data/freply
  FarObject{ { 0x3c05, 0x035d }, REPLY_017               }, // data/freply
  FarObject{ { 0x3c05, 0x0388 }, REPLY_018               }, // data/freply
  FarObject{ { 0x3c05, 0x03bb }, REPLY_019               }, // data/freply
  FarObject{ { 0x3c05, 0x03d6 }, REPLY_020               }, // data/freply
  FarObject{ { 0x3c05, 0x041d }, REPLY_021               }, // data/freply
  FarObject{ { 0x3c05, 0x041e }, REPLY_022               }, // data/freply
  FarObject{ { 0x3c05, 0x0454 }, REPLY_023               }, // data/freply
  FarObject{ { 0x3c05, 0x047f }, REPLY_024               }, // data/freply
  FarObject{ { 0x3c05, 0x04ba }, REPLY_025               }, // data/freply
  FarObject{ { 0x3c05, 0x04eb }, REPLY_026               }, // data/freply
  FarObject{ { 0x3c05, 0x0534 }, REPLY_027               }, // data/freply
  FarObject{ { 0x3c05, 0x0558 }, REPLY_028               }, // data/freply
  FarObject{ { 0x3c05, 0x057d }, REPLY_029               }, // data/freply
  FarObject{ { 0x3c05, 0x05a2 }, REPLY_030               }, // data/freply
  FarObject{ { 0x3c05, 0x05c6 }, REPLY_031               }, // data/freply
  FarObject{ { 0x3c05, 0x05ef }, REPLY_032               }, // data/freply
  FarObject{ { 0x3c05, 0x0621 }, REPLY_033               }, // data/freply
  FarObject{ { 0x3c05, 0x0639 }, REPLY_034               }, // data/freply
  FarObject{ { 0x3c05, 0x0659 }, REPLY_035               }, // data/freply
  FarObject{ { 0x3c05, 0x0679 }, REPLY_036               }, // data/freply
  FarObject{ { 0x3c05, 0x06ac }, REPLY_037               }, // data/freply
  FarObject{ { 0x3c05, 0x06e5 }, REPLY_038               }, // data/freply
  FarObject{ { 0x3c05, 0x0716 }, REPLY_039               }, // data/freply
  FarObject{ { 0x3c05, 0x0743 }, REPLY_040               }, // data/freply
  FarObject{ { 0x3c05, 0x0786 }, REPLY_041               }, // data/freply
  FarObject{ { 0x3c05, 0x07ab }, REPLY_042               }, // data/freply
  FarObject{ { 0x3c05, 0x07dd }, REPLY_043               }, // data/freply
  FarObject{ { 0x3c05, 0x07f6 }, REPLY_044               }, // data/freply
  FarObject{ { 0x3c05, 0x0822 }, REPLY_045               }, // data/freply
  FarObject{ { 0x3c05, 0x086d }, REPLY_046               }, // data/freply
  FarObject{ { 0x3c05, 0x08b5 }, REPLY_047               }, // data/freply
  FarObject{ { 0x3c05, 0x08ed }, REPLY_048               }, // data/freply
  FarObject{ { 0x3c05, 0x0924 }, REPLY_049               }, // data/freply
  FarObject{ { 0x3c05, 0x093c }, REPLY_050               }, // data/freply
  FarObject{ { 0x3c05, 0x0990 }, REPLY_051               }, // data/freply
  FarObject{ { 0x3c05, 0x09c2 }, REPLY_052               }, // data/freply
  FarObject{ { 0x3c05, 0x09f8 }, REPLY_053               }, // data/freply
  FarObject{ { 0x3c05, 0x0a33 }, REPLY_054               }, // data/freply
  FarObject{ { 0x3c05, 0x0a61 }, REPLY_055               }, // data/freply
  FarObject{ { 0x3c05, 0x0a97 }, REPLY_056               }, // data/freply
  FarObject{ { 0x3c05, 0x0aea }, REPLY_057               }, // data/freply
  FarObject{ { 0x3c05, 0x0afc }, REPLY_058               }, // data/freply
  FarObject{ { 0x3c05, 0x0b37 }, REPLY_059               }, // data/freply
  FarObject{ { 0x3c05, 0x0b53 }, REPLY_060               }, // data/freply
  FarObject{ { 0x3c05, 0x0b68 }, REPLY_061               }, // data/freply
  FarObject{ { 0x3c05, 0x0b9e }, REPLY_062               }, // data/freply
  FarObject{ { 0x3c05, 0x0bd3 }, REPLY_063               }, // data/freply
  FarObject{ { 0x3c05, 0x0c0f }, REPLY_064               }, // data/freply
  FarObject{ { 0x3c05, 0x0c50 }, REPLY_065               }, // data/freply
  FarObject{ { 0x3c05, 0x0c8d }, REPLY_066               }, // data/freply
  FarObject{ { 0x3c05, 0x0cd3 }, REPLY_067               }, // data/freply
  FarObject{ { 0x3c05, 0x0d11 }, REPLY_068               }, // data/freply
  FarObject{ { 0x3c05, 0x0d3f }, REPLY_069               }, // data/freply
  FarObject{ { 0x3c05, 0x0d6b }, REPLY_070               }, // data/freply
  FarObject{ { 0x3c05, 0x0db3 }, REPLY_071               }, // data/freply
  FarObject{ { 0x3c05, 0x0df6 }, REPLY_072               }, // data/freply
  FarObject{ { 0x3c05, 0x0e17 }, REPLY_073               }, // data/freply
  FarObject{ { 0x3c05, 0x0e40 }, REPLY_074               }, // data/freply
  FarObject{ { 0x3c05, 0x0e70 }, REPLY_075               }, // data/freply
  FarObject{ { 0x3c05, 0x0e94 }, REPLY_076               }, // data/freply
  FarObject{ { 0x3c05, 0x0ece }, REPLY_077               }, // data/freply
  FarObject{ { 0x3c05, 0x0f03 }, REPLY_078               }, // data/freply
  FarObject{ { 0x3c05, 0x0f38 }, REPLY_079               }, // data/freply
  FarObject{ { 0x3c05, 0x0f6e }, REPLY_080               }, // data/freply
  FarObject{ { 0x3c05, 0x0fb4 }, REPLY_081               }, // data/freply
  FarObject{ { 0x3c05, 0x100a }, REPLY_082               }, // data/freply
  FarObject{ { 0x3c05, 0x1040 }, REPLY_083               }, // data/freply
  FarObject{ { 0x3c05, 0x1078 }, REPLY_084               }, // data/freply
  FarObject{ { 0x3c05, 0x10a2 }, REPLY_085               }, // data/freply
  FarObject{ { 0x3c05, 0x10f7 }, REPLY_086               }, // data/freply
  FarObject{ { 0x3c05, 0x1149 }, REPLY_087               }, // data/freply
  FarObject{ { 0x3c05, 0x117a }, REPLY_088               }, // data/freply
  FarObject{ { 0x3c05, 0x11bf }, REPLY_089               }, // data/freply
  FarObject{ { 0x3c05, 0x1212 }, REPLY_090               }, // data/freply
  FarObject{ { 0x3c05, 0x1254 }, REPLY_091               }, // data/freply
  FarObject{ { 0x3c05, 0x12a6 }, REPLY_092               }, // data/freply
  FarObject{ { 0x3c05, 0x12eb }, REPLY_093               }, // data/freply
  FarObject{ { 0x3c05, 0x133a }, REPLY_094               }, // data/freply
  FarObject{ { 0x3c05, 0x1370 }, REPLY_095               }, // data/freply
  FarObject{ { 0x3c05, 0x139d }, REPLY_096               }, // data/freply
  FarObject{ { 0x3c05, 0x13bc }, REPLY_097               }, // data/freply
  FarObject{ { 0x3c05, 0x13e7 }, REPLY_098               }, // data/freply
  FarObject{ { 0x3c05, 0x1429 }, REPLY_099               }, // data/freply
  FarObject{ { 0x3c05, 0x143c }, REPLY_100               }, // data/freply
  FarObject{ { 0x3c05, 0x146c }, REPLY_101               }, // data/freply
  FarObject{ { 0x3c05, 0x148b }, REPLY_102               }, // data/freply
  FarObject{ { 0x3c05, 0x14ac }, REPLY_103               }, // data/freply
  FarObject{ { 0x3c05, 0x14c8 }, REPLY_104               }, // data/freply
  FarObject{ { 0x3c05, 0x14f0 }, REPLY_105               }, // data/freply
  FarObject{ { 0x3c05, 0x152a }, REPLY_106               }, // data/freply
  FarObject{ { 0x3c05, 0x1542 }, REPLY_107               }, // data/freply
  FarObject{ { 0x3c05, 0x156e }, REPLY_108               }, // data/freply
  FarObject{ { 0x3c05, 0x15ba }, REPLY_109               }, // data/freply
  FarObject{ { 0x3c05, 0x15de }, REPLY_110               }, // data/freply
  FarObject{ { 0x3c05, 0x1607 }, REPLY_111               }, // data/freply
  FarObject{ { 0x3c05, 0x1635 }, REPLY_112               }, // data/freply
  FarObject{ { 0x3c05, 0x1660 }, REPLY_113               }, // data/freply
  FarObject{ { 0x3c05, 0x1689 }, REPLY_114               }, // data/freply
  FarObject{ { 0x3c05, 0x16b5 }, REPLY_115               }, // data/freply
  FarObject{ { 0x3c05, 0x1700 }, REPLY_116               }, // data/freply
  FarObject{ { 0x3c05, 0x1736 }, REPLY_117               }, // data/freply
  FarObject{ { 0x3c05, 0x1759 }, REPLY_118               }, // data/freply
  FarObject{ { 0x3c05, 0x1776 }, REPLY_119               }, // data/freply
  FarObject{ { 0x3c05, 0x179f }, REPLY_120               }, // data/freply
  FarObject{ { 0x3c05, 0x17e8 }, REPLY_121               }, // data/freply
  FarObject{ { 0x3c05, 0x1817 }, REPLY_122               }, // data/freply
  FarObject{ { 0x3c05, 0x1856 }, REPLY_123               }, // data/freply
  FarObject{ { 0x3c05, 0x188a }, REPLY_124               }, // data/freply
  FarObject{ { 0x3d91, 0x0000 }, MUSIC_TRACKS            }, // data/fmusic
  FarObject{ { 0x3d91, 0x00be }, MUSIC_VOICES            }, // data/fmusic
  FarObject{ { 0x3d91, 0x071b }, MUSIC_EVENTS            }, // data/fmusic
  FarObject{ { 0x3d91, 0x7bba }, SOUND_EFFECTS           }, // data/fmusic
  FarObject{ { 0x4582, 0x0000 }, PATH_CELL_QUEUE         }, // data/fg4582
  FarObject{ { 0x4582, 0x0c80 }, PATH_COST_GRID          }, // data/fg4582
};
// clang-format on
} // namespace

auto FarDataBlocks() -> std::vector<ImageBlock> {
  return std::apply([](auto const&... objects) { return std::vector<ImageBlock>{ objects.Block()... }; }, FAR_OBJECTS);
}
} // namespace opeu::game::detail::image_block
