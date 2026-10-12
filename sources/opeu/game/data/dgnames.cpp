// Data segment DS:27be..3e6d (5808 bytes). Unit-type, weather, side, role, terrain and order names; resource
// filenames; initial turn/presentation variables; capacity weights; person templates; the 158 shared type/event
// records and their name-pointer tables.
#include "data/dgnames.hpp"
#include "data/image.hpp"

#include <array>
#include <cstddef>

namespace opeu::game::detail::dgnames {

auto InitialNames(DataSegment& data) -> void {
  data.save_last = 0x54;
  SetText(data.text_27c0, "Infantry I");
  SetText(data.text_27cb, "Infantry II");
  SetText(data.text_27d7, "Civil Grndier");
  SetText(data.text_27e5, "Combat Enginr");
  SetText(data.text_27f3, "Paratroop");
  SetText(data.text_27fd, "APC HalfTrack");
  SetText(data.text_280b, "Grenadier I");
  SetText(data.text_2817, "Grenadier II");
  SetText(data.text_2824, "Pzkpfw I");
  SetText(data.text_282d, "Pzkpfw II");
  SetText(data.text_2837, "Pzkpfw 35t");
  SetText(data.text_2842, "Pzkpfw 38t");
  SetText(data.text_284d, "Pzkpfw IIID");
  SetText(data.text_2859, "Pzkpfw IIIE");
  SetText(data.text_2865, "Pzkpfw IIIJ");
  SetText(data.text_2871, "Pzkpfw IIIL");
  SetText(data.text_287d, "Pzkpfw IIIM");
  SetText(data.text_2889, "Pzkpfw IVD");
  SetText(data.text_2894, "Pzkpfw IVF1");
  SetText(data.text_28a0, "Pzkpfw IVF2");
  SetText(data.text_28ac, "Pzkpfw IVG");
  SetText(data.text_28b7, "Pzkpfw IVH");
  SetText(data.text_28c2, "Panther D");
  SetText(data.text_28cc, "Panther G");
  SetText(data.text_28d6, "Tiger I");
  SetText(data.text_28de, "Tiger II");
  SetText(data.text_28e7, "StuG IIIB");
  SetText(data.text_28f1, "StuG IIIF");
  SetText(data.text_28fb, "StuG IIIG/75");
  SetText(data.text_2908, "StuG IIIG/105");
  SetText(data.text_2916, "StuG IVG");
  SetText(data.text_291f, "StuG IVL70");
  SetText(data.text_292a, "Jagdpanther");
  SetText(data.text_2936, "Elefant");
  SetText(data.text_293e, "Jagdtiger");
  SetText(data.text_2948, "Marder II");
  SetText(data.text_2952, "Nashorne");
  SetText(data.text_295b, "Hetzer");
  SetText(data.text_2962, "Hetzer D");
  SetText(data.text_296b, "Brummbar");
  SetText(data.text_2974, "Wespe");
  SetText(data.text_297a, "Hummel");
  SetText(data.text_2981, "Maultier");
  SetText(data.text_298a, "37mmPAK");
  SetText(data.text_2992, "50mmPAK");
  SetText(data.text_299a, "75mmPAK");
  SetText(data.text_29a2, "88mmPAK");
  SetText(data.text_29aa, "105mm Guns");
  SetText(data.text_29b5, "128mm Guns");
  SetText(data.text_29c0, "150mm Guns");
  SetText(data.text_29cb, "170mm Guns");
  SetText(data.text_29d6, "Nebelwerfer");
  SetText(data.text_29e2, "Infantry I");
  SetText(data.text_29ed, "Infantry II");
  SetText(data.text_29f9, "Paratroop");
  SetText(data.text_2a03, "Infantry");
  SetText(data.text_2a0c, "L3");
  SetText(data.text_2a0f, "M13/40");
  SetText(data.text_2a16, "Semovente 75/34");
  SetText(data.text_2a26, "P40");
  SetText(data.text_2a2a, "75/27");
  SetText(data.text_2a30, "75/32");
  SetText(data.text_2a36, "149mm Guns");
  SetText(data.text_2a41, "Paratroop");
  SetText(data.text_2a4b, "APC HalfTrack");
  SetText(data.text_2a59, "Chaffee");
  SetText(data.text_2a61, "M36");
  SetText(data.text_2a65, "Pershing");
  SetText(data.text_2a6e, "105mm Self-Prop");
  SetText(data.text_2a7e, "155mm Self-Prop");
  SetText(data.text_2a8e, "75mm Guns");
  SetText(data.text_2a98, "105mm Guns");
  SetText(data.text_2aa3, "155mm Guns");
  SetText(data.text_2aae, "Infantry I");
  SetText(data.text_2ab9, "Royal Inf I");
  SetText(data.text_2ac5, "Royal Inf II");
  SetText(data.text_2ad2, "Grenadier I");
  SetText(data.text_2ade, "Grenadier II");
  SetText(data.text_2aeb, "A13mk II");
  SetText(data.text_2af4, "Matilda I");
  SetText(data.text_2afe, "Matilda II");
  SetText(data.text_2b09, "Valentine");
  SetText(data.text_2b13, "Crusader I");
  SetText(data.text_2b1e, "Crusader II");
  SetText(data.text_2b2a, "Churchill III");
  SetText(data.text_2b38, "Churchill IV");
  SetText(data.text_2b45, "Lee");
  SetText(data.text_2b49, "Cromwell");
  SetText(data.text_2b52, "Firefly");
  SetText(data.text_2b5a, "6lb Guns");
  SetText(data.text_2b63, "17lb Guns");
  SetText(data.text_2b6d, "18lb Guns");
  SetText(data.text_2b77, "25lb Guns");
  SetText(data.text_2b81, "Stuart");
  SetText(data.text_2b88, "Grant");
  SetText(data.text_2b8e, "Sherman A1");
  SetText(data.text_2b99, "Sherman A1/76");
  SetText(data.text_2ba7, "Sherman A3");
  SetText(data.text_2bb2, "Sherman A3/76");
  SetText(data.text_2bc0, "Sherman A3/105");
  SetText(data.text_2bcf, "Sherman A3E8");
  SetText(data.text_2bdc, "Infantry");
  SetText(data.text_2be5, "APC HalfTrack");
  SetText(data.text_2bf3, "H35");
  SetText(data.text_2bf7, "Somua S35");
  SetText(data.text_2c01, "Char B");
  SetText(data.text_2c08, "75mm Guns");
  SetText(data.text_2c12, "155mm Guns");
  SetText(data.text_2c1d, "Infantry");
  SetText(data.text_2c26, "Sp. Guards");
  SetText(data.text_2c31, "Paratroop");
  SetText(data.text_2c3b, "APC HalfTrack");
  SetText(data.text_2c49, "KV1");
  SetText(data.text_2c4d, "KV2");
  SetText(data.text_2c51, "KV85");
  SetText(data.text_2c56, "T34/76C");
  SetText(data.text_2c5e, "T34/85");
  SetText(data.text_2c65, "JS1");
  SetText(data.text_2c69, "JS2");
  SetText(data.text_2c6d, "SU76");
  SetText(data.text_2c72, "SU85");
  SetText(data.text_2c77, "SU100");
  SetText(data.text_2c7d, "SU122");
  SetText(data.text_2c83, "SU152");
  SetText(data.text_2c89, "JSU122");
  SetText(data.text_2c90, "JSU152");
  SetText(data.text_2c97, "45mmPAK");
  SetText(data.text_2c9f, "57mmPAK");
  SetText(data.text_2ca7, "76mmPAK");
  SetText(data.text_2caf, "85mmPAK");
  SetText(data.text_2cb7, "120mmMTR");
  SetText(data.text_2cc0, "122mm Guns");
  SetText(data.text_2ccb, "152mm Guns");
  SetText(data.text_2cd6, "Katiuchas");
  SetText(data.text_2ce0, "Clear");
  SetText(data.text_2ce6, "Cloudy");
  SetText(data.text_2ced, "Rain");
  SetText(data.text_2cf2, "Snow");
  SetText(data.text_2cf7, "Fog");
  SetText(data.text_2cfb, "Stormy");
  SetText(data.text_2d02, "Axis");
  SetText(data.text_2d07, "Allied");
  SetText(data.text_2d0e, "GER");
  SetText(data.text_2d12, "ITY");
  SetText(data.text_2d16, "USA");
  SetText(data.text_2d1a, "ENG");
  SetText(data.text_2d1e, "FRN");
  SetText(data.text_2d22, "RUS");
  SetText(data.text_2d26, "Army Cmndr");
  SetText(data.text_2d31, "Ops Chief");
  SetText(data.text_2d3b, "Intl Chief");
  SetText(data.text_2d46, "Supreme Cmndr");
  SetText(data.text_2d54, "Chief of Staff");
  SetText(data.text_2d63, "Air Force Cmndr");
  SetText(data.text_2d73, "Special Forces Cmndr");
  SetText(data.text_2d88, "Battle");
  SetText(data.text_2d8f, "Battle");
  SetText(data.text_2d96, "Battle");
  SetText(data.text_2d9d, "Battle");
  SetText(data.text_2da4, "Assault");
  SetText(data.text_2dac, "Assault");
  SetText(data.text_2db4, "Enginr");
  SetText(data.text_2dbb, "Supply");
  SetText(data.text_2dc2, "Can't enter");
  SetText(data.text_2dce, "City");
  SetText(data.text_2dd3, "Road");
  SetText(data.text_2dd8, "Plains");
  SetText(data.text_2ddf, "Desert");
  SetText(data.text_2de6, "River");
  SetText(data.text_2dec, "Swamp");
  SetText(data.text_2df2, "Wasteland");
  SetText(data.text_2dfc, "Forest");
  SetText(data.text_2e03, "Hill");
  SetText(data.text_2e08, "Bridge");
  SetText(data.text_2e0f, "Occupy");
  SetText(data.text_2e16, "Defend");
  SetText(data.text_2e1d, "Move");
  SetText(data.text_2e22, "Move");
  SetText(data.text_2e27, "Occupy");
  SetText(data.text_2e2e, "Attack");
  SetText(data.text_2e35, "Defend");
  SetText(data.text_2e3c, "Mine");
  SetText(data.text_2e41, "Mine");
  SetText(data.text_2e46, "Bridge");
  SetText(data.text_2e4d, "Destroy");
  SetText(data.text_2e55, "Supply");
  SetText(data.text_2e5c, "Supply");
  SetText(data.text_2e63, "Repair");
  SetText(data.text_2e6a, "Repair");
  SetText(data.text_2e71, "Pursue");
  SetText(data.text_2e78, "Rest");
  SetText(data.text_2e7d, "Ammo");
  SetText(data.text_2e82, "Fuel");
  SetText(data.text_2e87, "Food");
  SetText(data.scenario_filename, "SCEDATA");
  SetText(data.face_file_name, "FACE.DAT");
  SetText(data.end_data_name, "SCEEND.DAT");
  SetText(data.text_2eaa, "KOEILOGO.FNT");
  SetText(data.scensel_file, "SCESEL.DAT");
  SetText(data.mgamen_filename, "MGAMEN.DAT");
  SetText(data.arms_file_name, "ARMS.SIL");
  SetText(data.midmap_filename, "MIDMAP.DAT");
  SetText(data.bigmap_filename, "BIGMAP.DAT");
  SetText(data.map_filename, "MAPINF.DAT");
  SetText(data.mappno_filename, "MAPPNO.DAT");
  SetText(data.hexmap_filename, "HEXMAP.DAT");
  SetText(data.hexdisp_filename, "HEXDISP.DAT");
  SetText(data.hexunit_filename, "HEXUNIT.DAT");
  SetText(data.hexfont_filename, "HEXFONT.DAT");
  SetText(data.hexfonts_filename, "HEXFONTS.DAT");
  SetText(data.msgrp_filename, "MSGRP.DAT");
  SetText(data.event_file_name, "EVENT.DAT");
  SetText(data.text_2f5c, "DCOM.OVL");
  SetText(data.text_2f66, "BMP.DAT");
  data.picture_handle         = 0xffff;
  data.portrait_handle        = 0xffff;
  data.face_handle            = 0xffff;
  data.message_picture_handle = 0xffff;
  data.unknown_2f76 = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                          0xff, 0xff, 0xff } };
  data.outcome_side = 0xff;
  data.outcome_reason    = 0xff;
  data.daily_pending     = 0x0001;
  data.overview_side     = 0xff;
  data.view_side         = 0xff;
  data.viewport_x        = 0xffff;
  data.viewport_y        = 0xffff;
  data.turn_frame_mode   = 0xff;
  data.context_side      = 0xff;
  data.presentation_mode = 0x0001;
  data.battle_map_x      = 0xff;
  data.battle_map_y      = 0xff;
  data.battle_attacker_x = 0xff;
  data.battle_attacker_y = 0xff;
  data.battle_direction  = 0xff;
  data.battle_report     = 0x0001;
  SetEach(std::to_array<std::uint16_t>(
              { 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
                0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
                0x0000, 0x0000, 0x55aa, 0x55aa, 0x55aa, 0x55aa }),
          [&](std::size_t index, std::uint16_t value) {
            data.battle_lists[index] = value;
          });
  data.overview_objective_glyph = { { 0x00, 0x00, 0x3c, 0x3c, 0x3c, 0x3c } };
  data.flash_marker = { { 0x18, 0x18, 0x18, 0xff, 0xff, 0x18, 0x18, 0x18 } };
  SetEach(std::to_array<std::uint16_t>(
              { 0x0000, 0x0079, 0x0158, 0x02d3, 0x045c, 0x05f7, 0x0795, 0x090e,
                0x0987, 0x0a52, 0x0b21, 0x0bf4, 0x0d09, 0x0d78, 0x0e04, 0x0e8b,
                0x0f19, 0x0fc9, 0x107b, 0x1134, 0x11ee, 0x12ad, 0x135e, 0x1415,
                0x14d3, 0x1596, 0x167b, 0x174a, 0x1817, 0x18f8, 0x19bc, 0x1a59,
                0x1afe, 0x1ba8, 0x1c54, 0x1ce4, 0x1d7c, 0x1e3e, 0x1f27, 0x1ff9,
                0x20a9, 0x2188, 0x2214, 0x229f, 0x235c, 0x240b, 0x24e0, 0x257d,
                0x25db, 0x2646, 0x26b9, 0x274a, 0x27bf, 0x2831, 0x28c6, 0x2965,
                0x29d4, 0x2a4f, 0x2b01, 0x2c79, 0x2df6, 0x2e6f, 0x2fed, 0x3165,
                0x3270, 0x32f2, 0x33cf, 0x34aa, 0x3566, 0x35e1, 0x3664, 0x3700,
                0x3798, 0x3872, 0x39f0, 0x3a7b, 0x3b1d, 0x3be7, 0x3c74, 0x3d15,
                0x3dad, 0x3e51, 0x3eff, 0x3f4e, 0x3fde, 0x408c, 0x413a, 0x41b8,
                0x433a, 0x44be, 0x4580, 0x461f, 0x468c, 0x4712, 0x482d, 0x48e9,
                0x498a, 0x4a22, 0x4aca, 0x4b93, 0x4c57, 0x4d35, 0x4e08, 0x4eb7,
                0x4f53, 0x4fe2, 0x504b, 0x50c7, 0x5148, 0x51d3, 0x5267, 0x5316,
                0x53a8, 0x543d, 0x54d6, 0x5572, 0x5608, 0x56b0, 0x574e, 0x57a9,
                0x592a, 0x59dc, 0x5a8f, 0x5b52, 0x5bcb, 0x5c68, 0x5d39, 0x5d9b,
                0x5e16, 0x5eab, 0x5f3b, 0x60d2, 0x624d, 0x63ca, 0x645f, 0x64f1,
                0x65fc, 0x66f3, 0x67ff, 0x68d9, 0x6989, 0x6a3f, 0x6b1f, 0x6c01,
                0x6ccb, 0x6d88, 0x6e4c, 0x6efa, 0x6fd1, 0x7072, 0x7112, 0x717e,
                0x71ec, 0x725d, 0x72ff, 0x739b, 0x7422, 0x74c2, 0x7581 }),
          [&](std::size_t index, std::uint16_t value) {
            data.face_offsets[index] = value;
          });
  data.ai_unit_element_counts = { { 0x06, 0x06, 0x06, 0x06, 0x04, 0x04, 0x04,
                                    0x04 } };
  SetEach(std::to_array<std::uint16_t>({ 0x03e7, 0x0063, 0x0063, 0x0063, 0x0063,
                                         0x0063, 0x0063, 0x0063 }),
          [&](std::size_t index, std::uint16_t value) {
            data.request_capacities[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x000a, 0x0032, 0x0032, 0x0032, 0x0032,
                                         0x001e, 0x001e, 0x0096 }),
          [&](std::size_t index, std::uint16_t value) {
            data.capacity_a_weights[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x000a, 0x0032, 0x001e, 0x001e, 0x001e,
                                         0x001e, 0x001e, 0x0096 }),
          [&](std::size_t index, std::uint16_t value) {
            data.capacity_c_weights[index] = value;
          });
  data.person_templates  = { { { { { "Hitler" } },
                                 0x03,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x5e,
                                 { 0x04 } },
                               { { { "Eisenhower" } },
                                 0x1c,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x60,
                                 { 0x04 } },
                               { { { "Stalin" } },
                                 0x20,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x62,
                                 { 0x04 } },
                               { { { "Halder" } },
                                 0x03,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x5f,
                                 { 0x05 } },
                               { { { "Smith" } },
                                 0x1c,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x61,
                                 { 0x05 } },
                               { { { "Vasilevsky" } },
                                 0x20,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x63,
                                 { 0x05 } },
                               { { { "Goring" } },
                                 0x03,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x64,
                                 { 0x06 } },
                               { { { "Spaatz" } },
                                 0x1c,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x65,
                                 { 0x06 } },
                               { { { "Novikov" } },
                                 0x20,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x66,
                                 { 0x06 } },
                               { { { "Donitz" } },
                                 0x03,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x67,
                                 { 0x07 } },
                               { { { "Laycock" } },
                                 0x1c,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x68,
                                 { 0x07 } },
                               { { { "Mayakov" } },
                                 0x20,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0x00,
                                 0xff,
                                 0x69,
                                 { 0x07 } } } };
  data.weather_obscurity = { { 0x00, 0x01, 0x03, 0x07, 0x03, 0x09 } };
  data.air_cover         = { { 0x00, 0x07, 0x01, 0x01, 0x02, 0x00, 0x00, 0x03,
                               0x06, 0x05, 0x01, 0x00, 0x00, 0x01, 0x01, 0x02,
                               0x03, 0x09, 0x08, 0x05, 0x06, 0x06, 0x01 } };
  data.battle_levels = { { 0x05, 0x08, 0x07, 0x09, 0x03, 0x03, 0x03, 0x03, 0x03,
                           0x03, 0x03, 0x03, 0x05 } };
  SetText(data.text_3300, "Engineer Unit");
  SetText(data.text_330e, "Repair Unit");
  SetText(data.text_331a, "Supply Unit");
  SetText(data.text_3326, "Mechanized");
  data.daily_records = { { { 0x331a,
                             0x01,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x01,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x27c0,
                             0x01,
                             0x00,
                             0x00,
                             { { 0x10, 0x0a, 0x0a, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x27cb,
                             0x01,
                             0x31,
                             0x00,
                             { { 0x16, 0x0f, 0x0f, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x27d7,
                             0x01,
                             0x41,
                             0x00,
                             { { 0x0e, 0x07, 0x05, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x27e5,
                             0x01,
                             0x00,
                             0x00,
                             { { 0x1e, 0x19, 0x14, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x27f3,
                             0x01,
                             0x00,
                             0x00,
                             { { 0x18, 0x14, 0x14, 0x05, 0x00, 0x00, 0x01 } },
                             0x01 },
                           { 0x3326,
                             0x01,
                             0x00,
                             0x01,
                             { { 0x10, 0x0f, 0x0f, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x27fd,
                             0x01,
                             0x00,
                             0x02,
                             { { 0x10, 0x0f, 0x19, 0x32, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x280b,
                             0x01,
                             0x21,
                             0x02,
                             { { 0x12, 0x14, 0x1e, 0x32, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x2817,
                             0x01,
                             0x31,
                             0x02,
                             { { 0x1c, 0x14, 0x23, 0x32, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x01,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x32, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x2824,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x0a, 0x0a, 0x06, 0x28, 0x52, 0x11, 0x01 } },
                             0x10 },
                           { 0x282d,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x0e, 0x0b, 0x0f, 0x28, 0x70, 0x11, 0x01 } },
                             0x10 },
                           { 0x2837,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x17, 0x15, 0x0f, 0x28, 0x7e, 0x0f, 0x01 } },
                             0x10 },
                           { 0x2842,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x1c, 0x15, 0x17, 0x38, 0x5c, 0x16, 0x01 } },
                             0x10 },
                           { 0x284d,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x1a, 0x15, 0x19, 0x28, 0x34, 0x20, 0x01 } },
                             0x00 },
                           { 0x2859,
                             0x01,
                             0x11,
                             0x03,
                             { { 0x26, 0x15, 0x19, 0x28, 0x34, 0x20, 0x01 } },
                             0x00 },
                           { 0x2865,
                             0x01,
                             0x16,
                             0x03,
                             { { 0x2f, 0x1d, 0x1b, 0x28, 0x34, 0x20, 0x01 } },
                             0x00 },
                           { 0x2871,
                             0x01,
                             0x21,
                             0x03,
                             { { 0x2f, 0x1d, 0x1d, 0x28, 0x30, 0x20, 0x01 } },
                             0x00 },
                           { 0x287d,
                             0x01,
                             0x26,
                             0x03,
                             { { 0x2f, 0x1d, 0x1c, 0x28, 0x30, 0x20, 0x01 } },
                             0x00 },
                           { 0x2889,
                             0x01,
                             0x00,
                             0x03,
                             { { 0x29, 0x2b, 0x0e, 0x2a, 0x2d, 0x2f, 0x01 } },
                             0x00 },
                           { 0x2894,
                             0x01,
                             0x11,
                             0x03,
                             { { 0x29, 0x2b, 0x13, 0x28, 0x2d, 0x2f, 0x01 } },
                             0x00 },
                           { 0x28a0,
                             0x01,
                             0x21,
                             0x03,
                             { { 0x39, 0x2b, 0x13, 0x28, 0x2d, 0x2f, 0x01 } },
                             0x00 },
                           { 0x28ac,
                             0x01,
                             0x26,
                             0x03,
                             { { 0x39, 0x2b, 0x18, 0x26, 0x2d, 0x2f, 0x01 } },
                             0x00 },
                           { 0x28b7,
                             0x01,
                             0x31,
                             0x03,
                             { { 0x3c, 0x2b, 0x1d, 0x26, 0x2d, 0x2f, 0x01 } },
                             0x00 },
                           { 0x28c2,
                             0x01,
                             0x36,
                             0x03,
                             { { 0x52, 0x2b, 0x28, 0x37, 0x23, 0x47, 0x01 } },
                             0x00 },
                           { 0x28cc,
                             0x01,
                             0x43,
                             0x03,
                             { { 0x52, 0x2b, 0x2e, 0x37, 0x23, 0x47, 0x01 } },
                             0x00 },
                           { 0x28d6,
                             0x01,
                             0x2b,
                             0x03,
                             { { 0x4d, 0x33, 0x31, 0x28, 0x12, 0x38, 0x01 } },
                             0x00 },
                           { 0x28de,
                             0x01,
                             0x45,
                             0x03,
                             { { 0x62, 0x33, 0x53, 0x26, 0x0f, 0x57, 0x01 } },
                             0x00 },
                           { 0x28e7,
                             0x01,
                             0x11,
                             0x03,
                             { { 0x25, 0x2b, 0x22, 0x28, 0x32, 0x1f, 0x01 } },
                             0x04 },
                           { 0x28f1,
                             0x01,
                             0x21,
                             0x03,
                             { { 0x2d, 0x2b, 0x23, 0x28, 0x32, 0x1f, 0x01 } },
                             0x04 },
                           { 0x28fb,
                             0x01,
                             0x31,
                             0x03,
                             { { 0x33, 0x2b, 0x24, 0x28, 0x32, 0x1f, 0x01 } },
                             0x04 },
                           { 0x2908,
                             0x01,
                             0x2a,
                             0x03,
                             { { 0x29, 0x3d, 0x24, 0x28, 0x32, 0x1f, 0x01 } },
                             0x04 },
                           { 0x2916,
                             0x01,
                             0x41,
                             0x03,
                             { { 0x33, 0x2b, 0x1e, 0x28, 0x2d, 0x2f, 0x01 } },
                             0x04 },
                           { 0x291f,
                             0x01,
                             0x45,
                             0x03,
                             { { 0x4a, 0x2b, 0x28, 0x23, 0x2d, 0x2f, 0x01 } },
                             0x04 },
                           { 0x292a,
                             0x01,
                             0x42,
                             0x03,
                             { { 0x58, 0x33, 0x29, 0x37, 0x24, 0x47, 0x01 } },
                             0x04 },
                           { 0x2936,
                             0x01,
                             0x37,
                             0x03,
                             { { 0x58, 0x33, 0x5b, 0x14, 0x0e, 0x6c, 0x01 } },
                             0x04 },
                           { 0x293e,
                             0x01,
                             0x4a,
                             0x03,
                             { { 0x63, 0x4a, 0x63, 0x23, 0x0f, 0x57, 0x01 } },
                             0x04 },
                           { 0x2948,
                             0x01,
                             0x23,
                             0x03,
                             { { 0x30, 0x2b, 0x0f, 0x38, 0x5c, 0x16, 0x01 } },
                             0x04 },
                           { 0x2952,
                             0x01,
                             0x31,
                             0x03,
                             { { 0x58, 0x33, 0x0f, 0x28, 0x32, 0x1f, 0x01 } },
                             0x04 },
                           { 0x295b,
                             0x01,
                             0x46,
                             0x03,
                             { { 0x33, 0x2b, 0x1d, 0x28, 0x63, 0x12, 0x01 } },
                             0x04 },
                           { 0x2962,
                             0x01,
                             0x49,
                             0x03,
                             { { 0x4a, 0x2b, 0x1d, 0x28, 0x63, 0x12, 0x01 } },
                             0x04 },
                           { 0x296b,
                             0x01,
                             0x41,
                             0x03,
                             { { 0x17, 0x57, 0x2d, 0x28, 0x2d, 0x2f, 0x01 } },
                             0x04 },
                           { 0x2974,
                             0x01,
                             0x21,
                             0x03,
                             { { 0x28, 0x3d, 0x06, 0x23, 0x64, 0x14, 0x06 } },
                             0x0c },
                           { 0x297a,
                             0x01,
                             0x31,
                             0x03,
                             { { 0x2d, 0x57, 0x09, 0x28, 0x64, 0x14, 0x07 } },
                             0x0c },
                           { 0x2981,
                             0x01,
                             0x41,
                             0x03,
                             { { 0x2e, 0x5a, 0x06, 0x32, 0x96, 0x14, 0x05 } },
                             0x0e },
                           { 0x298a,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x10, 0x15, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2992,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x26, 0x1d, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x299a,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x2b, 0x2b, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x29a2,
                             0x01,
                             0x11,
                             0x04,
                             { { 0x4f, 0x33, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x29aa,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x28, 0x3d, 0x00, 0x19, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x29b5,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x2b, 0x4a, 0x00, 0x19, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x29c0,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x2d, 0x57, 0x00, 0x19, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x29cb,
                             0x01,
                             0x00,
                             0x04,
                             { { 0x2f, 0x63, 0x00, 0x19, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x29d6,
                             0x01,
                             0x41,
                             0x04,
                             { { 0x2e, 0x5a, 0x00, 0x23, 0x64, 0x14, 0x05 } },
                             0x0a },
                           { 0x331a,
                             0x02,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x02,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x29e2,
                             0x02,
                             0x00,
                             0x00,
                             { { 0x0a, 0x05, 0x05, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x29ed,
                             0x02,
                             0x00,
                             0x00,
                             { { 0x0f, 0x05, 0x0a, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x3326,
                             0x02,
                             0x00,
                             0x01,
                             { { 0x0a, 0x0a, 0x0a, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x29f9,
                             0x08,
                             0x00,
                             0x00,
                             { { 0x16, 0x14, 0x16, 0x05, 0x00, 0x00, 0x01 } },
                             0x01 },
                           { 0x2a03,
                             0x04,
                             0x00,
                             0x00,
                             { { 0x10, 0x0a, 0x0a, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x02,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x32, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a0c,
                             0x02,
                             0x06,
                             0x03,
                             { { 0x0a, 0x0a, 0x06, 0x2a, 0xcb, 0x07, 0x01 } },
                             0x10 },
                           { 0x2a0f,
                             0x02,
                             0x0c,
                             0x03,
                             { { 0x17, 0x1b, 0x11, 0x1e, 0x69, 0x13, 0x01 } },
                             0x00 },
                           { 0x2a16,
                             0x02,
                             0x26,
                             0x03,
                             { { 0x1d, 0x2b, 0x11, 0x28, 0x69, 0x13, 0x01 } },
                             0x04 },
                           { 0x2a26,
                             0x02,
                             0x26,
                             0x03,
                             { { 0x28, 0x2b, 0x1a, 0x28, 0x45, 0x28, 0x01 } },
                             0x00 },
                           { 0x2a2a,
                             0x02,
                             0x00,
                             0x04,
                             { { 0x19, 0x2b, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a30,
                             0x02,
                             0x00,
                             0x04,
                             { { 0x1f, 0x2b, 0x0f, 0x1e, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a36,
                             0x02,
                             0x00,
                             0x04,
                             { { 0x30, 0x57, 0x00, 0x19, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x331a,
                             0x04,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x04,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a41,
                             0x04,
                             0x00,
                             0x00,
                             { { 0x18, 0x14, 0x14, 0x05, 0x00, 0x00, 0x01 } },
                             0x01 },
                           { 0x3326,
                             0x04,
                             0x00,
                             0x01,
                             { { 0x1a, 0x14, 0x0f, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a4b,
                             0x04,
                             0x00,
                             0x02,
                             { { 0x1a, 0x14, 0x23, 0x48, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x04,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x48, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x2a59,
                             0x04,
                             0x31,
                             0x03,
                             { { 0x2c, 0x2b, 0x11, 0x46, 0x3c, 0x32, 0x01 } },
                             0x10 },
                           { 0x2a61,
                             0x04,
                             0x47,
                             0x03,
                             { { 0x47, 0x34, 0x1d, 0x2a, 0x22, 0x49, 0x01 } },
                             0x00 },
                           { 0x2a65,
                             0x04,
                             0x51,
                             0x03,
                             { { 0x47, 0x34, 0x34, 0x2c, 0x19, 0x46, 0x01 } },
                             0x00 },
                           { 0x2a6e,
                             0x04,
                             0x00,
                             0x03,
                             { { 0x2c, 0x01, 0x0f, 0x28, 0x32, 0x32, 0x06 } },
                             0x0c },
                           { 0x2a7e,
                             0x04,
                             0x00,
                             0x03,
                             { { 0x31, 0x5a, 0x0f, 0x26, 0x32, 0x32, 0x07 } },
                             0x0c },
                           { 0x2a8e,
                             0x04,
                             0x00,
                             0x04,
                             { { 0x32, 0x2b, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2a98,
                             0x04,
                             0x00,
                             0x04,
                             { { 0x2c, 0x3d, 0x00, 0x23, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x2aa3,
                             0x04,
                             0x00,
                             0x04,
                             { { 0x31, 0x5a, 0x00, 0x23, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x331a,
                             0x08,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x08,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2aae,
                             0x08,
                             0x00,
                             0x00,
                             { { 0x10, 0x0a, 0x0a, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x2ab9,
                             0x08,
                             0x00,
                             0x00,
                             { { 0x12, 0x14, 0x0f, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x2ac5,
                             0x08,
                             0x31,
                             0x01,
                             { { 0x18, 0x14, 0x0f, 0x50, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x3326,
                             0x08,
                             0x00,
                             0x01,
                             { { 0x10, 0x0f, 0x0f, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2ad2,
                             0x08,
                             0x00,
                             0x02,
                             { { 0x12, 0x14, 0x1e, 0x3c, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x2ade,
                             0x08,
                             0x31,
                             0x02,
                             { { 0x1a, 0x14, 0x23, 0x3c, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x08,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x3c, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x2aeb,
                             0x08,
                             0x00,
                             0x03,
                             { { 0x23, 0x17, 0x09, 0x32, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2af4,
                             0x08,
                             0x05,
                             0x03,
                             { { 0x0a, 0x0a, 0x1e, 0x0c, 0x32, 0x0a, 0x01 } },
                             0x00 },
                           { 0x2afe,
                             0x08,
                             0x05,
                             0x03,
                             { { 0x20, 0x17, 0x24, 0x18, 0x33, 0x16, 0x01 } },
                             0x00 },
                           { 0x2b09,
                             0x08,
                             0x16,
                             0x03,
                             { { 0x20, 0x17, 0x20, 0x18, 0x6a, 0x0e, 0x01 } },
                             0x00 },
                           { 0x2b13,
                             0x08,
                             0x16,
                             0x03,
                             { { 0x20, 0x17, 0x0e, 0x2b, 0x20, 0x32, 0x01 } },
                             0x00 },
                           { 0x2b1e,
                             0x08,
                             0x26,
                             0x03,
                             { { 0x20, 0x17, 0x10, 0x2b, 0x20, 0x32, 0x01 } },
                             0x00 },
                           { 0x2b2a,
                             0x08,
                             0x28,
                             0x03,
                             { { 0x26, 0x21, 0x2b, 0x19, 0x16, 0x44, 0x01 } },
                             0x00 },
                           { 0x2b38,
                             0x08,
                             0x38,
                             0x03,
                             { { 0x2c, 0x2b, 0x2d, 0x16, 0x16, 0x44, 0x01 } },
                             0x00 },
                           { 0x2b45,
                             0x08,
                             0x2b,
                             0x03,
                             { { 0x28, 0x2b, 0x1a, 0x28, 0x1d, 0x46, 0x01 } },
                             0x00 },
                           { 0x2b49,
                             0x08,
                             0x46,
                             0x03,
                             { { 0x2c, 0x2b, 0x18, 0x33, 0x34, 0x35, 0x01 } },
                             0x00 },
                           { 0x2b52,
                             0x08,
                             0x46,
                             0x03,
                             { { 0x44, 0x2c, 0x1e, 0x26, 0x1b, 0x4d, 0x01 } },
                             0x00 },
                           { 0x2b5a,
                             0x08,
                             0x00,
                             0x04,
                             { { 0x20, 0x21, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2b63,
                             0x08,
                             0x21,
                             0x04,
                             { { 0x38, 0x2b, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2b6d,
                             0x08,
                             0x00,
                             0x04,
                             { { 0x2a, 0x2c, 0x00, 0x23, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x2b77,
                             0x08,
                             0x00,
                             0x04,
                             { { 0x2a, 0x33, 0x00, 0x23, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x2b81,
                             0x0c,
                             0x1b,
                             0x03,
                             { { 0x1f, 0x15, 0x12, 0x3a, 0x35, 0x15, 0x01 } },
                             0x10 },
                           { 0x2b88,
                             0x0c,
                             0x25,
                             0x03,
                             { { 0x24, 0x2b, 0x19, 0x23, 0x24, 0x41, 0x01 } },
                             0x00 },
                           { 0x2b8e,
                             0x0c,
                             0x2b,
                             0x03,
                             { { 0x2c, 0x2b, 0x19, 0x26, 0x18, 0x50, 0x01 } },
                             0x00 },
                           { 0x2b99,
                             0x0c,
                             0x3c,
                             0x03,
                             { { 0x3e, 0x2c, 0x19, 0x26, 0x18, 0x50, 0x01 } },
                             0x00 },
                           { 0x2ba7,
                             0x0c,
                             0x46,
                             0x03,
                             { { 0x2c, 0x2b, 0x23, 0x26, 0x1b, 0x4d, 0x01 } },
                             0x00 },
                           { 0x2bb2,
                             0x0c,
                             0x46,
                             0x03,
                             { { 0x3e, 0x2c, 0x23, 0x26, 0x1b, 0x4d, 0x01 } },
                             0x00 },
                           { 0x2bc0,
                             0x0c,
                             0x46,
                             0x03,
                             { { 0x26, 0x3d, 0x23, 0x26, 0x1b, 0x4d, 0x01 } },
                             0x00 },
                           { 0x2bcf,
                             0x0c,
                             0x49,
                             0x03,
                             { { 0x3e, 0x2c, 0x25, 0x26, 0x1b, 0x4d, 0x01 } },
                             0x00 },
                           { 0x331a,
                             0x10,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x10,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2bdc,
                             0x10,
                             0x00,
                             0x00,
                             { { 0x10, 0x0a, 0x0a, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x3326,
                             0x10,
                             0x00,
                             0x01,
                             { { 0x10, 0x0f, 0x0f, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2be5,
                             0x10,
                             0x00,
                             0x02,
                             { { 0x10, 0x0f, 0x1e, 0x3c, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x10,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x3c, 0x96, 0x14, 0x01 } },
                             0x00 },
                           { 0x2bf3,
                             0x10,
                             0x00,
                             0x03,
                             { { 0x0c, 0x15, 0x17, 0x1c, 0x19, 0x10, 0x01 } },
                             0x10 },
                           { 0x2bf7,
                             0x10,
                             0x00,
                             0x03,
                             { { 0x27, 0x1b, 0x17, 0x28, 0x53, 0x1c, 0x01 } },
                             0x00 },
                           { 0x2c01,
                             0x10,
                             0x00,
                             0x03,
                             { { 0x21, 0x2b, 0x14, 0x1c, 0x1d, 0x2d, 0x01 } },
                             0x00 },
                           { 0x2c08,
                             0x10,
                             0x00,
                             0x04,
                             { { 0x22, 0x2b, 0x0f, 0x23, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2c12,
                             0x10,
                             0x00,
                             0x04,
                             { { 0x31, 0x5a, 0x00, 0x1e, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x331a,
                             0x20,
                             0x00,
                             0x07,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x96, 0x01 } },
                             0x00 },
                           { 0x330e,
                             0x20,
                             0x00,
                             0x06,
                             { { 0x01, 0x01, 0x00, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2c1d,
                             0x20,
                             0x00,
                             0x00,
                             { { 0x12, 0x0f, 0x0f, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x2c26,
                             0x20,
                             0x31,
                             0x00,
                             { { 0x14, 0x14, 0x14, 0x05, 0x00, 0x00, 0x01 } },
                             0x00 },
                           { 0x2c31,
                             0x20,
                             0x00,
                             0x00,
                             { { 0x10, 0x0f, 0x14, 0x05, 0x00, 0x00, 0x01 } },
                             0x01 },
                           { 0x3326,
                             0x20,
                             0x31,
                             0x01,
                             { { 0x10, 0x0f, 0x0f, 0x50, 0xc8, 0x14, 0x01 } },
                             0x00 },
                           { 0x2c3b,
                             0x20,
                             0x31,
                             0x02,
                             { { 0x12, 0x14, 0x23, 0x48, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x3300,
                             0x20,
                             0x00,
                             0x05,
                             { { 0x0a, 0x05, 0x0f, 0x48, 0x96, 0x18, 0x01 } },
                             0x00 },
                           { 0x2c49,
                             0x20,
                             0x00,
                             0x03,
                             { { 0x2e, 0x2c, 0x28, 0x1c, 0x2a, 0x3c, 0x01 } },
                             0x00 },
                           { 0x2c4d,
                             0x20,
                             0x00,
                             0x03,
                             { { 0x31, 0x58, 0x41, 0x1a, 0x26, 0x36, 0x01 } },
                             0x00 },
                           { 0x2c51,
                             0x20,
                             0x38,
                             0x03,
                             { { 0x45, 0x31, 0x41, 0x28, 0x1a, 0x60, 0x01 } },
                             0x00 },
                           { 0x2c56,
                             0x20,
                             0x16,
                             0x03,
                             { { 0x32, 0x2c, 0x23, 0x32, 0x49, 0x3d, 0x01 } },
                             0x00 },
                           { 0x2c5e,
                             0x20,
                             0x36,
                             0x03,
                             { { 0x45, 0x31, 0x24, 0x32, 0x50, 0x36, 0x01 } },
                             0x00 },
                           { 0x2c65,
                             0x20,
                             0x41,
                             0x03,
                             { { 0x45, 0x31, 0x4a, 0x25, 0x1e, 0x50, 0x01 } },
                             0x00 },
                           { 0x2c69,
                             0x20,
                             0x51,
                             0x03,
                             { { 0x52, 0x47, 0x4a, 0x25, 0x1d, 0x53, 0x01 } },
                             0x00 },
                           { 0x2c6d,
                             0x20,
                             0x2c,
                             0x03,
                             { { 0x2c, 0x2c, 0x08, 0x32, 0x13, 0x3f, 0x01 } },
                             0x04 },
                           { 0x2c72,
                             0x20,
                             0x38,
                             0x03,
                             { { 0x3e, 0x31, 0x21, 0x36, 0x3a, 0x33, 0x01 } },
                             0x04 },
                           { 0x2c77,
                             0x20,
                             0x48,
                             0x03,
                             { { 0x4b, 0x3a, 0x21, 0x36, 0x3a, 0x33, 0x01 } },
                             0x04 },
                           { 0x2c7d,
                             0x20,
                             0x32,
                             0x03,
                             { { 0x4a, 0x47, 0x21, 0x36, 0x3a, 0x33, 0x01 } },
                             0x04 },
                           { 0x2c83,
                             0x20,
                             0x32,
                             0x03,
                             { { 0x3e, 0x58, 0x32, 0x28, 0x2a, 0x3c, 0x01 } },
                             0x04 },
                           { 0x2c89,
                             0x20,
                             0x48,
                             0x03,
                             { { 0x4a, 0x47, 0x4a, 0x25, 0x1d, 0x53, 0x01 } },
                             0x04 },
                           { 0x2c90,
                             0x20,
                             0x48,
                             0x03,
                             { { 0x3e, 0x58, 0x4a, 0x25, 0x1d, 0x53, 0x01 } },
                             0x04 },
                           { 0x2c97,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x1a, 0x1a, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2c9f,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x2b, 0x21, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2ca7,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x32, 0x2c, 0x0f, 0x28, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2caf,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x3b, 0x31, 0x0f, 0x23, 0x64, 0x14, 0x01 } },
                             0x00 },
                           { 0x2cb7,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x26, 0x3c, 0x0f, 0x05, 0x00, 0x00, 0x02 } },
                             0x08 },
                           { 0x2cc0,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x2e, 0x47, 0x00, 0x23, 0x64, 0x14, 0x06 } },
                             0x08 },
                           { 0x2ccb,
                             0x20,
                             0x00,
                             0x04,
                             { { 0x31, 0x58, 0x00, 0x23, 0x64, 0x14, 0x07 } },
                             0x08 },
                           { 0x2cd6,
                             0x20,
                             0x41,
                             0x04,
                             { { 0x28, 0x46, 0x00, 0x50, 0xc8, 0x14, 0x05 } },
                             0x0a } } };
  SetEach(std::to_array<std::uint16_t>(
              { 0x2ce0, 0x2ce6, 0x2ced, 0x2cf2, 0x2cf7, 0x2cfb }),
          [&](std::size_t index, std::uint16_t value) {
            data.weather_names[index] = value;
          });
  data.ui_side_names[0] = 0x2d02;
  data.ui_side_names[1] = 0x2d07;
  SetEach(std::to_array<std::uint16_t>(
              { 0x2d0e, 0x2d12, 0x2d16, 0x2d1a, 0x2d1e, 0x2d22 }),
          [&](std::size_t index, std::uint16_t value) {
            data.mask_names[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>(
              { 0x2d26, 0x2d31, 0x2d3b, 0x2d46, 0x2d54, 0x2d63, 0x2d73 }),
          [&](std::size_t index, std::uint16_t value) {
            data.role_names[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x2d88, 0x2d8f, 0x2d96, 0x2d9d, 0x2da4,
                                         0x2dac, 0x2db4, 0x2dbb }),
          [&](std::size_t index, std::uint16_t value) {
            data.entry_names[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x2dc2, 0x2dce, 0x2dd3, 0x2dd8, 0x2ddf,
                                         0x2de6, 0x2dec, 0x2df2, 0x2dfc, 0x2e03,
                                         0x2e08 }),
          [&](std::size_t index, std::uint16_t value) {
            data.cursor_code_text[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x2e0f, 0x2e16, 0x2e1d, 0x2e22, 0x2e27,
                                         0x2e2e, 0x2e35, 0x2e3c, 0x2e41, 0x2e46,
                                         0x2e4d, 0x2e55, 0x2e5c, 0x2e63, 0x2e6a,
                                         0x2e71, 0x2e78 }),
          [&](std::size_t index, std::uint16_t value) {
            data.order_names[index] = value;
          });
  data.supply_names[0] = 0x2e7d;
  data.supply_names[1] = 0x2e82;
  data.supply_names[2] = 0x2e87;
  data.battle_digits   = {
    { 0x30, 0x48, 0x48, 0x48, 0x48, 0x30, 0x10, 0x30, 0x10, 0x10, 0x10,
      0x38, 0x30, 0x48, 0x08, 0x10, 0x20, 0x78, 0x78, 0x08, 0x30, 0x08,
      0x48, 0x30, 0x10, 0x30, 0x50, 0x50, 0x78, 0x10, 0x78, 0x40, 0x70,
      0x08, 0x08, 0x70, 0x30, 0x40, 0x70, 0x48, 0x48, 0x30, 0x78, 0x48,
      0x10, 0x20, 0x20, 0x20, 0x30, 0x48, 0x30, 0x48, 0x48, 0x30, 0x30,
      0x48, 0x48, 0x38, 0x08, 0x30, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc }
  };
  data.rotated_glyphs = {
    { 0x3c, 0x42, 0x42, 0x3c, 0x00, 0x02, 0x7e, 0x22, 0x00, 0x00, 0x32,
      0x4a, 0x46, 0x22, 0x00, 0x6c, 0x52, 0x52, 0x44, 0x00, 0x04, 0x7e,
      0x24, 0x1c, 0x04, 0x4c, 0x52, 0x52, 0x72, 0x00, 0x0c, 0x52, 0x52,
      0x3c, 0x00, 0x60, 0x50, 0x4f, 0x60, 0x00, 0x2c, 0x52, 0x52, 0x2c,
      0x00, 0x3c, 0x4a, 0x4a, 0x30, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff }
  };
  SetText(data.text_3c2a, "Move");
  SetText(data.text_3c2f, "Attack");
  SetText(data.text_3c36, "Blitz");
  SetText(data.text_3c3c, "Withdraw");
  SetText(data.text_3c45, "Defend");
  SetText(data.text_3c4c, "Wait");
  SetText(data.text_3c51, "Road");
  SetText(data.text_3c56, "Plain");
  SetText(data.text_3c5c, "Desert");
  SetText(data.text_3c63, "River");
  SetText(data.text_3c69, "Swamp");
  SetText(data.text_3c6f, "Rough");
  SetText(data.text_3c75, "Forest");
  SetText(data.text_3c7c, "Hills");
  SetText(data.text_3c82, "Bridge");
  SetText(data.text_3c89, "Building");
  SetText(data.text_3c92, "Bunker");
  data.battle_cells = {
    { { { 0x60, 0x60, 0x60, 0x00, 0x00, 0x70, 0x70, 0x70, 0x70, 0x00, 0x00,
          0x80, 0x80, 0x80 } },
      { { 0x60, 0x60, 0x60, 0x00, 0x00, 0x70, 0x70, 0x70, 0x70, 0x00, 0x00,
          0x80, 0x80, 0x80 } },
      { { 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x80, 0x80 } },
      { { 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
      { { 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
      { { 0x40, 0x40, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
          0x00, 0x50, 0x50 } },
      { { 0x40, 0x40, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
          0x00, 0x50, 0x50 } },
      { { 0x40, 0x40, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
          0x00, 0x50, 0x50 } },
      { { 0x40, 0x40, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
          0x00, 0x50, 0x50 } },
      { { 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
      { { 0x00, 0x00, 0x00, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 } },
      { { 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
          0x00, 0x30, 0x30 } },
      { { 0x10, 0x10, 0x10, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
          0x30, 0x30, 0x30 } },
      { { 0x10, 0x10, 0x10, 0x00, 0x00, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00,
          0x30, 0x30, 0x30 } } }
  };
  data.battle_numbers_shown  = 0x01;
  data.battle_ai_entry_side  = { { 0xff } };
  data.battle_type_factors   = { { 0x1e, 0x1e, 0x1e, 0x28, 0x3c, 0x1e, 0x1e,
                                   0x1e } };
  data.battle_spot_range     = { { 0x04, 0x03, 0x03, 0x02, 0x01, 0x01, 0x01,
                                   0x01 } };
  data.battle_terrain_extras = {
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
  };
  SetEach(std::to_array<std::uint16_t>(
              { 0x3c2a, 0x3c2f, 0x3c36, 0x3c3c, 0x3c45, 0x3c4c }),
          [&](std::size_t index, std::uint16_t value) {
            data.battle_order_names[index] = value;
          });
  SetEach(std::to_array<std::uint16_t>({ 0x3c51, 0x3c56, 0x3c5c, 0x3c63, 0x3c69,
                                         0x3c6f, 0x3c75, 0x3c7c, 0x3c82, 0x3c89,
                                         0x3c92 }),
          [&](std::size_t index, std::uint16_t value) {
            data.battle_terrain_names[index] = value;
          });
  data.battle_terrain_costs = {
    { 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x06,
      0x05, 0x08, 0x08, 0x0c, 0x0c, 0x07, 0x18, 0x0d, 0x09, 0x14, 0x0e,
      0x24, 0x24, 0x0a, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x08,
      0x1c, 0x13, 0x11, 0x1c, 0x19, 0x2a, 0x2a, 0x07, 0x0c, 0x0a, 0x07,
      0x0c, 0x0b, 0x12, 0x12, 0x06, 0x0a, 0x09, 0x08, 0x0a, 0x0c, 0x0f,
      0x0f, 0x07, 0x08, 0x07, 0x07, 0x09, 0x0b, 0x0c, 0x0c, 0x05, 0x05,
      0x05, 0x05, 0x05, 0x08, 0x08, 0x08, 0x09, 0xff, 0xff, 0xff, 0xff,
      0xff, 0xff, 0xff, 0x0a, 0xff, 0xff, 0xff, 0x12, 0xff, 0xff, 0xff }
  };
  data.battle_terrain_cover = 0x01;
  data.battle_visibility    = 0x01;
  data.unknown_3e58         = { { 0x02, 0x00, 0x00, 0x03, 0x06, 0x05, 0x01,
                                  0x07, 0x09, 0x00, 0x01, 0x02, 0x03, 0x09,
                                  0x08, 0x05, 0x06, 0x06, 0x01, 0x04, 0x04 } };
}
} // namespace opeu::game::detail::dgnames
