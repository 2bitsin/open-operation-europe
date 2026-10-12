// The segment:offset of every object the data modules (game/data) define. Only their addresses and sizes are taken
// here; their types and contents stay in the data modules. The scenario workspace is written by data/dgscen.
#include "port.hpp"

#include <iterator>

extern unsigned char opeu_view_cell_tiles[];
extern unsigned char opeu_turn_frame_short_tiles[];
extern unsigned char opeu_turn_frame_tall_tiles[];
extern unsigned char opeu_element_type_icons[];
extern unsigned char opeu_battle_hex_masks[];
extern unsigned char opeu_yes_no_pointer_save[];
extern unsigned char opeu_text_cursor_tile[];
extern unsigned char opeu_yes_no_pointer_image[];
extern unsigned char opeu_yes_no_pointer_shape[];
extern unsigned char opeu_menu_button_tiles[];
extern unsigned char opeu_setup_marker_save[];
extern unsigned char opeu_setup_marker_image[];
extern unsigned char opeu_overview_type_masks[];
extern unsigned char opeu_overview_type_sprites[];
extern unsigned char opeu_overview_order_masks[];
extern unsigned char opeu_overview_order_sprites[];
extern unsigned char opeu_overview_blast_frames[];
extern unsigned char opeu_view_type_sprites[];
extern unsigned char opeu_view_type_masks[];
extern unsigned char opeu_view_shadow_images[];
extern unsigned char opeu_view_order_sprites[];
extern unsigned char opeu_view_order_masks[];
extern unsigned char opeu_view_blast_sprites[];
extern unsigned char opeu_view_blast_masks[];
extern unsigned char opeu_battle_report_0[];
extern unsigned char opeu_battle_report_1[];
extern unsigned char opeu_battle_report_2[];
extern unsigned char opeu_battle_report_3[];
extern unsigned char opeu_battle_report_4[];
extern unsigned char opeu_battle_report_5[];
extern unsigned char opeu_battle_report_6[];
extern unsigned char opeu_context_frame_tiles[];
extern unsigned char opeu_list_arrow_tiles[];
extern unsigned char opeu_war_news_000[];
extern unsigned char opeu_war_news_001[];
extern unsigned char opeu_war_news_002[];
extern unsigned char opeu_war_news_003[];
extern unsigned char opeu_war_news_004[];
extern unsigned char opeu_war_news_005[];
extern unsigned char opeu_war_news_006[];
extern unsigned char opeu_war_news_007[];
extern unsigned char opeu_war_news_008[];
extern unsigned char opeu_war_news_009[];
extern unsigned char opeu_war_news_010[];
extern unsigned char opeu_war_news_011[];
extern unsigned char opeu_war_news_012[];
extern unsigned char opeu_war_news_013[];
extern unsigned char opeu_war_news_014[];
extern unsigned char opeu_war_news_015[];
extern unsigned char opeu_war_news_016[];
extern unsigned char opeu_war_news_017[];
extern unsigned char opeu_war_news_018[];
extern unsigned char opeu_war_news_019[];
extern unsigned char opeu_war_news_020[];
extern unsigned char opeu_war_news_021[];
extern unsigned char opeu_war_news_022[];
extern unsigned char opeu_war_news_023[];
extern unsigned char opeu_war_news_024[];
extern unsigned char opeu_war_news_025[];
extern unsigned char opeu_war_news_026[];
extern unsigned char opeu_war_news_027[];
extern unsigned char opeu_war_news_028[];
extern unsigned char opeu_war_news_029[];
extern unsigned char opeu_war_news_030[];
extern unsigned char opeu_war_news_031[];
extern unsigned char opeu_war_news_032[];
extern unsigned char opeu_war_news_033[];
extern unsigned char opeu_war_news_034[];
extern unsigned char opeu_war_news_035[];
extern unsigned char opeu_war_news_036[];
extern unsigned char opeu_war_news_037[];
extern unsigned char opeu_war_news_038[];
extern unsigned char opeu_war_news_039[];
extern unsigned char opeu_war_news_040[];
extern unsigned char opeu_war_news_041[];
extern unsigned char opeu_war_news_042[];
extern unsigned char opeu_war_news_043[];
extern unsigned char opeu_war_news_044[];
extern unsigned char opeu_war_news_045[];
extern unsigned char opeu_war_news_046[];
extern unsigned char opeu_war_news_047[];
extern unsigned char opeu_war_news_048[];
extern unsigned char opeu_war_news_049[];
extern unsigned char opeu_war_news_050[];
extern unsigned char opeu_war_news_051[];
extern unsigned char opeu_war_news_052[];
extern unsigned char opeu_war_news_053[];
extern unsigned char opeu_war_news_054[];
extern unsigned char opeu_war_news_055[];
extern unsigned char opeu_war_news_056[];
extern unsigned char opeu_war_news_057[];
extern unsigned char opeu_war_news_058[];
extern unsigned char opeu_war_news_059[];
extern unsigned char opeu_war_news_060[];
extern unsigned char opeu_war_news_061[];
extern unsigned char opeu_war_news_062[];
extern unsigned char opeu_war_news_063[];
extern unsigned char opeu_war_news_064[];
extern unsigned char opeu_war_news_065[];
extern unsigned char opeu_war_news_066[];
extern unsigned char opeu_war_news_067[];
extern unsigned char opeu_war_news_068[];
extern unsigned char opeu_war_news_069[];
extern unsigned char opeu_war_news_070[];
extern unsigned char opeu_war_news_071[];
extern unsigned char opeu_war_news_072[];
extern unsigned char opeu_war_news_073[];
extern unsigned char opeu_war_news_074[];
extern unsigned char opeu_war_news_075[];
extern unsigned char opeu_war_news_076[];
extern unsigned char opeu_war_news_077[];
extern unsigned char opeu_war_news_078[];
extern unsigned char opeu_war_news_079[];
extern unsigned char opeu_war_news_080[];
extern unsigned char opeu_war_news_081[];
extern unsigned char opeu_war_news_082[];
extern unsigned char opeu_war_news_083[];
extern unsigned char opeu_war_news_084[];
extern unsigned char opeu_war_news_085[];
extern unsigned char opeu_war_news_086[];
extern unsigned char opeu_war_news_087[];
extern unsigned char opeu_war_news_088[];
extern unsigned char opeu_war_news_089[];
extern unsigned char opeu_war_news_090[];
extern unsigned char opeu_war_news_091[];
extern unsigned char opeu_war_news_092[];
extern unsigned char opeu_war_news_093[];
extern unsigned char opeu_war_news_094[];
extern unsigned char opeu_war_news_095[];
extern unsigned char opeu_war_news_096[];
extern unsigned char opeu_war_news_097[];
extern unsigned char opeu_war_news_098[];
extern unsigned char opeu_war_news_099[];
extern unsigned char opeu_war_news_100[];
extern unsigned char opeu_war_news_101[];
extern unsigned char opeu_war_news_102[];
extern unsigned char opeu_war_news_103[];
extern unsigned char opeu_war_news_104[];
extern unsigned char opeu_war_news_105[];
extern unsigned char opeu_war_news_106[];
extern unsigned char opeu_war_news_107[];
extern unsigned char opeu_war_news_108[];
extern unsigned char opeu_war_news_109[];
extern unsigned char opeu_war_news_110[];
extern unsigned char opeu_war_news_111[];
extern unsigned char opeu_war_news_112[];
extern unsigned char opeu_war_news_113[];
extern unsigned char opeu_war_news_114[];
extern unsigned char opeu_war_news_115[];
extern unsigned char opeu_war_news_116[];
extern unsigned char opeu_war_news_117[];
extern unsigned char opeu_war_news_118[];
extern unsigned char opeu_war_news_119[];
extern unsigned char opeu_war_news_120[];
extern unsigned char opeu_war_news_121[];
extern unsigned char opeu_war_news_122[];
extern unsigned char opeu_war_news_123[];
extern unsigned char opeu_war_news_124[];
extern unsigned char opeu_war_news_125[];
extern unsigned char opeu_war_news_126[];
extern unsigned char opeu_war_news_127[];
extern unsigned char opeu_war_news_128[];
extern unsigned char opeu_war_news_129[];
extern unsigned char opeu_war_news_130[];
extern unsigned char opeu_war_news_131[];
extern unsigned char opeu_war_news_132[];
extern unsigned char opeu_war_news_133[];
extern unsigned char opeu_war_news_134[];
extern unsigned char opeu_war_news_135[];
extern unsigned char opeu_war_news_136[];
extern unsigned char opeu_war_news_137[];
extern unsigned char opeu_war_news_138[];
extern unsigned char opeu_war_news_139[];
extern unsigned char opeu_war_news_140[];
extern unsigned char opeu_war_news_141[];
extern unsigned char opeu_weather_terrain_ratings[];
extern unsigned char opeu_air_sprite_save[];
extern unsigned char opeu_bomb_sprite_save[];
extern unsigned char opeu_panel_border_tiles[];
extern unsigned char opeu_advice_00[];
extern unsigned char opeu_advice_01[];
extern unsigned char opeu_advice_02[];
extern unsigned char opeu_advice_03[];
extern unsigned char opeu_advice_04[];
extern unsigned char opeu_advice_05[];
extern unsigned char opeu_advice_06[];
extern unsigned char opeu_advice_07[];
extern unsigned char opeu_advice_08[];
extern unsigned char opeu_advice_09[];
extern unsigned char opeu_advice_10[];
extern unsigned char opeu_advice_11[];
extern unsigned char opeu_advice_12[];
extern unsigned char opeu_advice_13[];
extern unsigned char opeu_advice_14[];
extern unsigned char opeu_advice_15[];
extern unsigned char opeu_advice_16[];
extern unsigned char opeu_advice_17[];
extern unsigned char opeu_advice_18[];
extern unsigned char opeu_advice_19[];
extern unsigned char opeu_advice_20[];
extern unsigned char opeu_advice_21[];
extern unsigned char opeu_advice_22[];
extern unsigned char opeu_advice_23[];
extern unsigned char opeu_advice_24[];
extern unsigned char opeu_advice_25[];
extern unsigned char opeu_advice_26[];
extern unsigned char opeu_advice_27[];
extern unsigned char opeu_advice_28[];
extern unsigned char opeu_advice_29[];
extern unsigned char opeu_advice_30[];
extern unsigned char opeu_advice_31[];
extern unsigned char opeu_advice_32[];
extern unsigned char opeu_advice_33[];
extern unsigned char opeu_advice_34[];
extern unsigned char opeu_advice_35[];
extern unsigned char opeu_advice_36[];
extern unsigned char opeu_advice_37[];
extern unsigned char opeu_advice_38[];
extern unsigned char opeu_advice_39[];
extern unsigned char opeu_advice_40[];
extern unsigned char opeu_advice_41[];
extern unsigned char opeu_advice_42[];
extern unsigned char opeu_advice_43[];
extern unsigned char opeu_advice_44[];
extern unsigned char opeu_advice_45[];
extern unsigned char opeu_advice_46[];
extern unsigned char opeu_advice_47[];
extern unsigned char opeu_advice_48[];
extern unsigned char opeu_advice_49[];
extern unsigned char opeu_advice_50[];
extern unsigned char opeu_advice_51[];
extern unsigned char opeu_advice_52[];
extern unsigned char opeu_advice_53[];
extern unsigned char opeu_advice_54[];
extern unsigned char opeu_advice_55[];
extern unsigned char opeu_advice_56[];
extern unsigned char opeu_advice_57[];
extern unsigned char opeu_advice_58[];
extern unsigned char opeu_advice_59[];
extern unsigned char opeu_advice_60[];
extern unsigned char opeu_advice_61[];
extern unsigned char opeu_advice_62[];
extern unsigned char opeu_advice_63[];
extern unsigned char opeu_advice_64[];
extern unsigned char opeu_advice_65[];
extern unsigned char opeu_advice_66[];
extern unsigned char opeu_advice_67[];
extern unsigned char opeu_advice_68[];
extern unsigned char opeu_advice_69[];
extern unsigned char opeu_advice_70[];
extern unsigned char opeu_advice_71[];
extern unsigned char opeu_advice_72[];
extern unsigned char opeu_advice_73[];
extern unsigned char opeu_advice_74[];
extern unsigned char opeu_advice_75[];
extern unsigned char opeu_advice_76[];
extern unsigned char opeu_advice_77[];
extern unsigned char opeu_advice_78[];
extern unsigned char opeu_advice_79[];
extern unsigned char opeu_advice_80[];
extern unsigned char opeu_advice_81[];
extern unsigned char opeu_advice_82[];
extern unsigned char opeu_advice_83[];
extern unsigned char opeu_advice_84[];
extern unsigned char opeu_advice_85[];
extern unsigned char opeu_advice_86[];
extern unsigned char opeu_advice_87[];
extern unsigned char opeu_advice_88[];
extern unsigned char opeu_advice_89[];
extern unsigned char opeu_advice_90[];
extern unsigned char opeu_advice_91[];
extern unsigned char opeu_advice_92[];
extern unsigned char opeu_reply_000[];
extern unsigned char opeu_reply_001[];
extern unsigned char opeu_reply_002[];
extern unsigned char opeu_reply_003[];
extern unsigned char opeu_reply_004[];
extern unsigned char opeu_reply_005[];
extern unsigned char opeu_reply_006[];
extern unsigned char opeu_reply_007[];
extern unsigned char opeu_reply_008[];
extern unsigned char opeu_reply_009[];
extern unsigned char opeu_reply_010[];
extern unsigned char opeu_reply_011[];
extern unsigned char opeu_reply_012[];
extern unsigned char opeu_reply_013[];
extern unsigned char opeu_reply_014[];
extern unsigned char opeu_reply_015[];
extern unsigned char opeu_reply_016[];
extern unsigned char opeu_reply_017[];
extern unsigned char opeu_reply_018[];
extern unsigned char opeu_reply_019[];
extern unsigned char opeu_reply_020[];
extern unsigned char opeu_reply_021[];
extern unsigned char opeu_reply_022[];
extern unsigned char opeu_reply_023[];
extern unsigned char opeu_reply_024[];
extern unsigned char opeu_reply_025[];
extern unsigned char opeu_reply_026[];
extern unsigned char opeu_reply_027[];
extern unsigned char opeu_reply_028[];
extern unsigned char opeu_reply_029[];
extern unsigned char opeu_reply_030[];
extern unsigned char opeu_reply_031[];
extern unsigned char opeu_reply_032[];
extern unsigned char opeu_reply_033[];
extern unsigned char opeu_reply_034[];
extern unsigned char opeu_reply_035[];
extern unsigned char opeu_reply_036[];
extern unsigned char opeu_reply_037[];
extern unsigned char opeu_reply_038[];
extern unsigned char opeu_reply_039[];
extern unsigned char opeu_reply_040[];
extern unsigned char opeu_reply_041[];
extern unsigned char opeu_reply_042[];
extern unsigned char opeu_reply_043[];
extern unsigned char opeu_reply_044[];
extern unsigned char opeu_reply_045[];
extern unsigned char opeu_reply_046[];
extern unsigned char opeu_reply_047[];
extern unsigned char opeu_reply_048[];
extern unsigned char opeu_reply_049[];
extern unsigned char opeu_reply_050[];
extern unsigned char opeu_reply_051[];
extern unsigned char opeu_reply_052[];
extern unsigned char opeu_reply_053[];
extern unsigned char opeu_reply_054[];
extern unsigned char opeu_reply_055[];
extern unsigned char opeu_reply_056[];
extern unsigned char opeu_reply_057[];
extern unsigned char opeu_reply_058[];
extern unsigned char opeu_reply_059[];
extern unsigned char opeu_reply_060[];
extern unsigned char opeu_reply_061[];
extern unsigned char opeu_reply_062[];
extern unsigned char opeu_reply_063[];
extern unsigned char opeu_reply_064[];
extern unsigned char opeu_reply_065[];
extern unsigned char opeu_reply_066[];
extern unsigned char opeu_reply_067[];
extern unsigned char opeu_reply_068[];
extern unsigned char opeu_reply_069[];
extern unsigned char opeu_reply_070[];
extern unsigned char opeu_reply_071[];
extern unsigned char opeu_reply_072[];
extern unsigned char opeu_reply_073[];
extern unsigned char opeu_reply_074[];
extern unsigned char opeu_reply_075[];
extern unsigned char opeu_reply_076[];
extern unsigned char opeu_reply_077[];
extern unsigned char opeu_reply_078[];
extern unsigned char opeu_reply_079[];
extern unsigned char opeu_reply_080[];
extern unsigned char opeu_reply_081[];
extern unsigned char opeu_reply_082[];
extern unsigned char opeu_reply_083[];
extern unsigned char opeu_reply_084[];
extern unsigned char opeu_reply_085[];
extern unsigned char opeu_reply_086[];
extern unsigned char opeu_reply_087[];
extern unsigned char opeu_reply_088[];
extern unsigned char opeu_reply_089[];
extern unsigned char opeu_reply_090[];
extern unsigned char opeu_reply_091[];
extern unsigned char opeu_reply_092[];
extern unsigned char opeu_reply_093[];
extern unsigned char opeu_reply_094[];
extern unsigned char opeu_reply_095[];
extern unsigned char opeu_reply_096[];
extern unsigned char opeu_reply_097[];
extern unsigned char opeu_reply_098[];
extern unsigned char opeu_reply_099[];
extern unsigned char opeu_reply_100[];
extern unsigned char opeu_reply_101[];
extern unsigned char opeu_reply_102[];
extern unsigned char opeu_reply_103[];
extern unsigned char opeu_reply_104[];
extern unsigned char opeu_reply_105[];
extern unsigned char opeu_reply_106[];
extern unsigned char opeu_reply_107[];
extern unsigned char opeu_reply_108[];
extern unsigned char opeu_reply_109[];
extern unsigned char opeu_reply_110[];
extern unsigned char opeu_reply_111[];
extern unsigned char opeu_reply_112[];
extern unsigned char opeu_reply_113[];
extern unsigned char opeu_reply_114[];
extern unsigned char opeu_reply_115[];
extern unsigned char opeu_reply_116[];
extern unsigned char opeu_reply_117[];
extern unsigned char opeu_reply_118[];
extern unsigned char opeu_reply_119[];
extern unsigned char opeu_reply_120[];
extern unsigned char opeu_reply_121[];
extern unsigned char opeu_reply_122[];
extern unsigned char opeu_reply_123[];
extern unsigned char opeu_reply_124[];
extern unsigned char opeu_music_tracks[];
extern unsigned char opeu_music_voices[];
extern unsigned char opeu_music_events[];
extern unsigned char opeu_sound_effects[];
extern unsigned char opeu_path_cell_queue[];
extern unsigned char opeu_path_cost_grid[];
extern unsigned char opeu_turn_order[];
extern unsigned char opeu_objective_links[];
extern unsigned char opeu_unit_links[];
extern unsigned char opeu_map_cells[];
extern unsigned char opeu_influence_map[];
extern unsigned char opeu_unit_type_names[];
extern unsigned char opeu_general_names[];
extern unsigned char opeu_resource_filenames[];
extern unsigned char opeu_turn_variables[];
extern unsigned char opeu_request_capacities[];
extern unsigned char opeu_capacity_a_weights[];
extern unsigned char opeu_capacity_c_weights[];
extern unsigned char opeu_person_templates[];
extern unsigned char opeu_template_tables[];
extern unsigned char opeu_element_types[];
extern unsigned char opeu_weather_names[];
extern unsigned char opeu_side_names[];
extern unsigned char opeu_mask_names[];
extern unsigned char opeu_role_names[];
extern unsigned char opeu_entry_names[];
extern unsigned char opeu_terrain_names[];
extern unsigned char opeu_order_names[];
extern unsigned char opeu_supply_names[];
extern unsigned char opeu_small_glyphs[];
extern unsigned char opeu_text_vars[];
extern unsigned char opeu_text_font[];
extern unsigned char opeu_setup_text[];
extern unsigned char opeu_startup_text[];
extern unsigned char opeu_label_text[];
extern unsigned char opeu_briefing_text[];
extern unsigned char opeu_side_initializers[];
extern unsigned char opeu_event_tables[];
extern unsigned char opeu_request_supplies_name[];
extern unsigned char opeu_request_battalions_name[];
extern unsigned char opeu_request_regiments_name[];
extern unsigned char opeu_request_kind_names[];
extern unsigned char opeu_request_delay_bases[];
extern unsigned char opeu_equipment_delivery_format[];
extern unsigned char opeu_request_weights2[];
extern unsigned char opeu_context_name_format[];
extern unsigned char opeu_context_stats_format[];
extern unsigned char opeu_context_blank_format[];
extern unsigned char opeu_context_text_padding[];
extern unsigned char opeu_scenario_contexts[];
extern unsigned char opeu_menu_pass_label[];
extern unsigned char opeu_menu_tact_label[];
extern unsigned char opeu_menu_city_label[];
extern unsigned char opeu_menu_tsfr_label[];
extern unsigned char opeu_menu_ofcr_label[];
extern unsigned char opeu_menu_info_label[];
extern unsigned char opeu_menu_orgz_label[];
extern unsigned char opeu_menu_rqst_label[];
extern unsigned char opeu_menu_air_label[];
extern unsigned char opeu_menu_spfc_label[];
extern unsigned char opeu_menu_game_label[];
extern unsigned char opeu_control_prompt[];
extern unsigned char opeu_control_ready[];
extern unsigned char opeu_control_text_padding[];
extern unsigned char opeu_control_actions[];
extern unsigned char opeu_control_menu[];
extern unsigned char opeu_turn_text[];
extern unsigned char opeu_order_text[];
extern unsigned char opeu_supply_text[];
extern unsigned char opeu_air_text[];
extern unsigned char opeu_combat_text[];
extern unsigned char opeu_battle_text[];

const PortImageObject port_image_objects[] = {
  { opeu_view_cell_tiles, 0x349bu, 0x0000u, 288u,
    "opeu_fg349b_0000" }, /* data/fg349b */
  { opeu_turn_frame_short_tiles, 0x349bu, 0x0120u, 576u,
    "opeu_fg349b_0120" }, /* data/fg349b */
  { opeu_turn_frame_tall_tiles, 0x349bu, 0x0360u, 768u,
    "opeu_fg349b_0360" }, /* data/fg349b */
  { opeu_element_type_icons, 0x349bu, 0x0660u, 704u,
    "opeu_fg349b_0660" }, /* data/fg349b */
  { opeu_battle_hex_masks, 0x352du, 0x0000u, 390u,
    "opeu_fg352d_0000" }, /* data/fg352d */
  { opeu_yes_no_pointer_save, 0x3546u, 0x0000u, 960u,
    "opeu_fg3546_0000" }, /* data/fg3546 */
  { opeu_text_cursor_tile, 0x3546u, 0x03c0u, 32u,
    "opeu_fg3546_03c0" }, /* data/fg3546 */
  { opeu_yes_no_pointer_image, 0x3546u, 0x03e0u, 768u,
    "opeu_fg3546_03e0" }, /* data/fg3546 */
  { opeu_yes_no_pointer_shape, 0x3546u, 0x06e0u, 192u,
    "opeu_fg3546_06e0" }, /* data/fg3546 */
  { opeu_menu_button_tiles, 0x35c0u, 0x0000u, 576u,
    "opeu_fg35c0_0000" }, /* data/fg35c0 */
  { opeu_setup_marker_save, 0x35e4u, 0x0000u, 512u,
    "opeu_fg35e4_0000" }, /* data/fg35e4 */
  { opeu_setup_marker_image, 0x35e4u, 0x0200u, 384u,
    "opeu_fg35e4_0200" }, /* data/fg35e4 */
  { opeu_overview_type_masks, 0x361cu, 0x0000u, 80u,
    "opeu_fg361c_0000" }, /* data/fg361c */
  { opeu_overview_type_sprites, 0x361cu, 0x0050u, 240u,
    "opeu_fg361c_0050" }, /* data/fg361c */
  { opeu_overview_order_masks, 0x361cu, 0x0140u, 72u,
    "opeu_fg361c_0140" }, /* data/fg361c */
  { opeu_overview_order_sprites, 0x361cu, 0x0188u, 216u,
    "opeu_fg361c_0188" }, /* data/fg361c */
  { opeu_overview_blast_frames, 0x361cu, 0x0260u, 48u,
    "opeu_fg361c_0260" }, /* data/fg361c */
  { opeu_view_type_sprites, 0x361cu, 0x0290u, 864u,
    "opeu_fg361c_0290" }, /* data/fg361c */
  { opeu_view_type_masks, 0x361cu, 0x05f0u, 2592u,
    "opeu_fg361c_05f0" }, /* data/fg361c */
  { opeu_view_shadow_images, 0x361cu, 0x1010u, 144u,
    "opeu_fg361c_1010" }, /* data/fg361c */
  { opeu_view_order_sprites, 0x361cu, 0x10a0u, 1944u,
    "opeu_fg361c_10a0" }, /* data/fg361c */
  { opeu_view_order_masks, 0x361cu, 0x1838u, 1944u,
    "opeu_fg361c_1838" }, /* data/fg361c */
  { opeu_view_blast_sprites, 0x361cu, 0x1fd0u, 216u,
    "opeu_fg361c_1fd0" }, /* data/fg361c */
  { opeu_view_blast_masks, 0x361cu, 0x20a8u, 648u,
    "opeu_fg361c_20a8" }, /* data/fg361c */
  { opeu_battle_report_0, 0x384fu, 0x0000u, 73u,
    "opeu_battle_report_0" }, /* data/fbattle */
  { opeu_battle_report_1, 0x384fu, 0x0049u, 125u,
    "opeu_battle_report_1" }, /* data/fbattle */
  { opeu_battle_report_2, 0x384fu, 0x00c6u, 125u,
    "opeu_battle_report_2" }, /* data/fbattle */
  { opeu_battle_report_3, 0x384fu, 0x0143u, 121u,
    "opeu_battle_report_3" }, /* data/fbattle */
  { opeu_battle_report_4, 0x384fu, 0x01bcu, 46u,
    "opeu_battle_report_4" }, /* data/fbattle */
  { opeu_battle_report_5, 0x384fu, 0x01eau, 48u,
    "opeu_battle_report_5" }, /* data/fbattle */
  { opeu_battle_report_6, 0x384fu, 0x021au, 38u,
    "opeu_battle_report_6" }, /* data/fbattle */
  { opeu_context_frame_tiles, 0x3873u, 0x0000u, 288u,
    "opeu_fg3873_0000" }, /* data/fg3873 */
  { opeu_list_arrow_tiles, 0x3885u, 0x0000u, 256u,
    "opeu_fg3885_0000" }, /* data/fg3885 */
  { opeu_war_news_000, 0x3895u, 0x0000u, 46u,
    "opeu_war_news_000" }, /* data/fnews */
  { opeu_war_news_001, 0x3895u, 0x002eu, 47u,
    "opeu_war_news_001" }, /* data/fnews */
  { opeu_war_news_002, 0x3895u, 0x005du, 59u,
    "opeu_war_news_002" }, /* data/fnews */
  { opeu_war_news_003, 0x3895u, 0x0098u, 56u,
    "opeu_war_news_003" }, /* data/fnews */
  { opeu_war_news_004, 0x3895u, 0x00d0u, 59u,
    "opeu_war_news_004" }, /* data/fnews */
  { opeu_war_news_005, 0x3895u, 0x010bu, 61u,
    "opeu_war_news_005" }, /* data/fnews */
  { opeu_war_news_006, 0x3895u, 0x0148u, 46u,
    "opeu_war_news_006" }, /* data/fnews */
  { opeu_war_news_007, 0x3895u, 0x0176u, 66u,
    "opeu_war_news_007" }, /* data/fnews */
  { opeu_war_news_008, 0x3895u, 0x01b8u, 48u,
    "opeu_war_news_008" }, /* data/fnews */
  { opeu_war_news_009, 0x3895u, 0x01e8u, 66u,
    "opeu_war_news_009" }, /* data/fnews */
  { opeu_war_news_010, 0x3895u, 0x022au, 60u,
    "opeu_war_news_010" }, /* data/fnews */
  { opeu_war_news_011, 0x3895u, 0x0266u, 54u,
    "opeu_war_news_011" }, /* data/fnews */
  { opeu_war_news_012, 0x3895u, 0x029cu, 41u,
    "opeu_war_news_012" }, /* data/fnews */
  { opeu_war_news_013, 0x3895u, 0x02c5u, 38u,
    "opeu_war_news_013" }, /* data/fnews */
  { opeu_war_news_014, 0x3895u, 0x02ebu, 61u,
    "opeu_war_news_014" }, /* data/fnews */
  { opeu_war_news_015, 0x3895u, 0x0328u, 68u,
    "opeu_war_news_015" }, /* data/fnews */
  { opeu_war_news_016, 0x3895u, 0x036cu, 49u,
    "opeu_war_news_016" }, /* data/fnews */
  { opeu_war_news_017, 0x3895u, 0x039du, 61u,
    "opeu_war_news_017" }, /* data/fnews */
  { opeu_war_news_018, 0x3895u, 0x03dau, 53u,
    "opeu_war_news_018" }, /* data/fnews */
  { opeu_war_news_019, 0x3895u, 0x040fu, 60u,
    "opeu_war_news_019" }, /* data/fnews */
  { opeu_war_news_020, 0x3895u, 0x044bu, 31u,
    "opeu_war_news_020" }, /* data/fnews */
  { opeu_war_news_021, 0x3895u, 0x046au, 62u,
    "opeu_war_news_021" }, /* data/fnews */
  { opeu_war_news_022, 0x3895u, 0x04a8u, 49u,
    "opeu_war_news_022" }, /* data/fnews */
  { opeu_war_news_023, 0x3895u, 0x04d9u, 62u,
    "opeu_war_news_023" }, /* data/fnews */
  { opeu_war_news_024, 0x3895u, 0x0517u, 68u,
    "opeu_war_news_024" }, /* data/fnews */
  { opeu_war_news_025, 0x3895u, 0x055bu, 43u,
    "opeu_war_news_025" }, /* data/fnews */
  { opeu_war_news_026, 0x3895u, 0x0586u, 35u,
    "opeu_war_news_026" }, /* data/fnews */
  { opeu_war_news_027, 0x3895u, 0x05a9u, 44u,
    "opeu_war_news_027" }, /* data/fnews */
  { opeu_war_news_028, 0x3895u, 0x05d5u, 69u,
    "opeu_war_news_028" }, /* data/fnews */
  { opeu_war_news_029, 0x3895u, 0x061au, 69u,
    "opeu_war_news_029" }, /* data/fnews */
  { opeu_war_news_030, 0x3895u, 0x065fu, 66u,
    "opeu_war_news_030" }, /* data/fnews */
  { opeu_war_news_031, 0x3895u, 0x06a1u, 63u,
    "opeu_war_news_031" }, /* data/fnews */
  { opeu_war_news_032, 0x3895u, 0x06e0u, 38u,
    "opeu_war_news_032" }, /* data/fnews */
  { opeu_war_news_033, 0x3895u, 0x0706u, 42u,
    "opeu_war_news_033" }, /* data/fnews */
  { opeu_war_news_034, 0x3895u, 0x0730u, 74u,
    "opeu_war_news_034" }, /* data/fnews */
  { opeu_war_news_035, 0x3895u, 0x077au, 77u,
    "opeu_war_news_035" }, /* data/fnews */
  { opeu_war_news_036, 0x3895u, 0x07c7u, 69u,
    "opeu_war_news_036" }, /* data/fnews */
  { opeu_war_news_037, 0x3895u, 0x080cu, 26u,
    "opeu_war_news_037" }, /* data/fnews */
  { opeu_war_news_038, 0x3895u, 0x0826u, 77u,
    "opeu_war_news_038" }, /* data/fnews */
  { opeu_war_news_039, 0x3895u, 0x0873u, 77u,
    "opeu_war_news_039" }, /* data/fnews */
  { opeu_war_news_040, 0x3895u, 0x08c0u, 64u,
    "opeu_war_news_040" }, /* data/fnews */
  { opeu_war_news_041, 0x3895u, 0x0900u, 45u,
    "opeu_war_news_041" }, /* data/fnews */
  { opeu_war_news_042, 0x3895u, 0x092du, 40u,
    "opeu_war_news_042" }, /* data/fnews */
  { opeu_war_news_043, 0x3895u, 0x0955u, 43u,
    "opeu_war_news_043" }, /* data/fnews */
  { opeu_war_news_044, 0x3895u, 0x0980u, 59u,
    "opeu_war_news_044" }, /* data/fnews */
  { opeu_war_news_045, 0x3895u, 0x09bbu, 27u,
    "opeu_war_news_045" }, /* data/fnews */
  { opeu_war_news_046, 0x3895u, 0x09d6u, 70u,
    "opeu_war_news_046" }, /* data/fnews */
  { opeu_war_news_047, 0x3895u, 0x0a1cu, 53u,
    "opeu_war_news_047" }, /* data/fnews */
  { opeu_war_news_048, 0x3895u, 0x0a51u, 82u,
    "opeu_war_news_048" }, /* data/fnews */
  { opeu_war_news_049, 0x3895u, 0x0aa3u, 61u,
    "opeu_war_news_049" }, /* data/fnews */
  { opeu_war_news_050, 0x3895u, 0x0ae0u, 55u,
    "opeu_war_news_050" }, /* data/fnews */
  { opeu_war_news_051, 0x3895u, 0x0b17u, 38u,
    "opeu_war_news_051" }, /* data/fnews */
  { opeu_war_news_052, 0x3895u, 0x0b3du, 48u,
    "opeu_war_news_052" }, /* data/fnews */
  { opeu_war_news_053, 0x3895u, 0x0b6du, 31u,
    "opeu_war_news_053" }, /* data/fnews */
  { opeu_war_news_054, 0x3895u, 0x0b8cu, 59u,
    "opeu_war_news_054" }, /* data/fnews */
  { opeu_war_news_055, 0x3895u, 0x0bc7u, 42u,
    "opeu_war_news_055" }, /* data/fnews */
  { opeu_war_news_056, 0x3895u, 0x0bf1u, 62u,
    "opeu_war_news_056" }, /* data/fnews */
  { opeu_war_news_057, 0x3895u, 0x0c2fu, 60u,
    "opeu_war_news_057" }, /* data/fnews */
  { opeu_war_news_058, 0x3895u, 0x0c6bu, 66u,
    "opeu_war_news_058" }, /* data/fnews */
  { opeu_war_news_059, 0x3895u, 0x0cadu, 76u,
    "opeu_war_news_059" }, /* data/fnews */
  { opeu_war_news_060, 0x3895u, 0x0cf9u, 48u,
    "opeu_war_news_060" }, /* data/fnews */
  { opeu_war_news_061, 0x3895u, 0x0d29u, 44u,
    "opeu_war_news_061" }, /* data/fnews */
  { opeu_war_news_062, 0x3895u, 0x0d55u, 51u,
    "opeu_war_news_062" }, /* data/fnews */
  { opeu_war_news_063, 0x3895u, 0x0d88u, 74u,
    "opeu_war_news_063" }, /* data/fnews */
  { opeu_war_news_064, 0x3895u, 0x0dd2u, 44u,
    "opeu_war_news_064" }, /* data/fnews */
  { opeu_war_news_065, 0x3895u, 0x0dfeu, 85u,
    "opeu_war_news_065" }, /* data/fnews */
  { opeu_war_news_066, 0x3895u, 0x0e53u, 60u,
    "opeu_war_news_066" }, /* data/fnews */
  { opeu_war_news_067, 0x3895u, 0x0e8fu, 42u,
    "opeu_war_news_067" }, /* data/fnews */
  { opeu_war_news_068, 0x3895u, 0x0eb9u, 55u,
    "opeu_war_news_068" }, /* data/fnews */
  { opeu_war_news_069, 0x3895u, 0x0ef0u, 37u,
    "opeu_war_news_069" }, /* data/fnews */
  { opeu_war_news_070, 0x3895u, 0x0f15u, 58u,
    "opeu_war_news_070" }, /* data/fnews */
  { opeu_war_news_071, 0x3895u, 0x0f4fu, 41u,
    "opeu_war_news_071" }, /* data/fnews */
  { opeu_war_news_072, 0x3895u, 0x0f78u, 42u,
    "opeu_war_news_072" }, /* data/fnews */
  { opeu_war_news_073, 0x3895u, 0x0fa2u, 60u,
    "opeu_war_news_073" }, /* data/fnews */
  { opeu_war_news_074, 0x3895u, 0x0fdeu, 40u,
    "opeu_war_news_074" }, /* data/fnews */
  { opeu_war_news_075, 0x3895u, 0x1006u, 54u,
    "opeu_war_news_075" }, /* data/fnews */
  { opeu_war_news_076, 0x3895u, 0x103cu, 52u,
    "opeu_war_news_076" }, /* data/fnews */
  { opeu_war_news_077, 0x3895u, 0x1070u, 32u,
    "opeu_war_news_077" }, /* data/fnews */
  { opeu_war_news_078, 0x3895u, 0x1090u, 77u,
    "opeu_war_news_078" }, /* data/fnews */
  { opeu_war_news_079, 0x3895u, 0x10ddu, 37u,
    "opeu_war_news_079" }, /* data/fnews */
  { opeu_war_news_080, 0x3895u, 0x1102u, 58u,
    "opeu_war_news_080" }, /* data/fnews */
  { opeu_war_news_081, 0x3895u, 0x113cu, 68u,
    "opeu_war_news_081" }, /* data/fnews */
  { opeu_war_news_082, 0x3895u, 0x1180u, 78u,
    "opeu_war_news_082" }, /* data/fnews */
  { opeu_war_news_083, 0x3895u, 0x11ceu, 52u,
    "opeu_war_news_083" }, /* data/fnews */
  { opeu_war_news_084, 0x3895u, 0x1202u, 37u,
    "opeu_war_news_084" }, /* data/fnews */
  { opeu_war_news_085, 0x3895u, 0x1227u, 79u,
    "opeu_war_news_085" }, /* data/fnews */
  { opeu_war_news_086, 0x3895u, 0x1276u, 79u,
    "opeu_war_news_086" }, /* data/fnews */
  { opeu_war_news_087, 0x3895u, 0x12c5u, 37u,
    "opeu_war_news_087" }, /* data/fnews */
  { opeu_war_news_088, 0x3895u, 0x12eau, 47u,
    "opeu_war_news_088" }, /* data/fnews */
  { opeu_war_news_089, 0x3895u, 0x1319u, 53u,
    "opeu_war_news_089" }, /* data/fnews */
  { opeu_war_news_090, 0x3895u, 0x134eu, 47u,
    "opeu_war_news_090" }, /* data/fnews */
  { opeu_war_news_091, 0x3895u, 0x137du, 49u,
    "opeu_war_news_091" }, /* data/fnews */
  { opeu_war_news_092, 0x3895u, 0x13aeu, 41u,
    "opeu_war_news_092" }, /* data/fnews */
  { opeu_war_news_093, 0x3895u, 0x13d7u, 43u,
    "opeu_war_news_093" }, /* data/fnews */
  { opeu_war_news_094, 0x3895u, 0x1402u, 28u,
    "opeu_war_news_094" }, /* data/fnews */
  { opeu_war_news_095, 0x3895u, 0x141eu, 68u,
    "opeu_war_news_095" }, /* data/fnews */
  { opeu_war_news_096, 0x3895u, 0x1462u, 36u,
    "opeu_war_news_096" }, /* data/fnews */
  { opeu_war_news_097, 0x3895u, 0x1486u, 56u,
    "opeu_war_news_097" }, /* data/fnews */
  { opeu_war_news_098, 0x3895u, 0x14beu, 62u,
    "opeu_war_news_098" }, /* data/fnews */
  { opeu_war_news_099, 0x3895u, 0x14fcu, 45u,
    "opeu_war_news_099" }, /* data/fnews */
  { opeu_war_news_100, 0x3895u, 0x1529u, 40u,
    "opeu_war_news_100" }, /* data/fnews */
  { opeu_war_news_101, 0x3895u, 0x1551u, 37u,
    "opeu_war_news_101" }, /* data/fnews */
  { opeu_war_news_102, 0x3895u, 0x1576u, 40u,
    "opeu_war_news_102" }, /* data/fnews */
  { opeu_war_news_103, 0x3895u, 0x159eu, 52u,
    "opeu_war_news_103" }, /* data/fnews */
  { opeu_war_news_104, 0x3895u, 0x15d2u, 81u,
    "opeu_war_news_104" }, /* data/fnews */
  { opeu_war_news_105, 0x3895u, 0x1623u, 39u,
    "opeu_war_news_105" }, /* data/fnews */
  { opeu_war_news_106, 0x3895u, 0x164au, 30u,
    "opeu_war_news_106" }, /* data/fnews */
  { opeu_war_news_107, 0x3895u, 0x1668u, 39u,
    "opeu_war_news_107" }, /* data/fnews */
  { opeu_war_news_108, 0x3895u, 0x168fu, 33u,
    "opeu_war_news_108" }, /* data/fnews */
  { opeu_war_news_109, 0x3895u, 0x16b0u, 43u,
    "opeu_war_news_109" }, /* data/fnews */
  { opeu_war_news_110, 0x3895u, 0x16dbu, 51u,
    "opeu_war_news_110" }, /* data/fnews */
  { opeu_war_news_111, 0x3895u, 0x170eu, 37u,
    "opeu_war_news_111" }, /* data/fnews */
  { opeu_war_news_112, 0x3895u, 0x1733u, 40u,
    "opeu_war_news_112" }, /* data/fnews */
  { opeu_war_news_113, 0x3895u, 0x175bu, 46u,
    "opeu_war_news_113" }, /* data/fnews */
  { opeu_war_news_114, 0x3895u, 0x1789u, 65u,
    "opeu_war_news_114" }, /* data/fnews */
  { opeu_war_news_115, 0x3895u, 0x17cau, 73u,
    "opeu_war_news_115" }, /* data/fnews */
  { opeu_war_news_116, 0x3895u, 0x1813u, 57u,
    "opeu_war_news_116" }, /* data/fnews */
  { opeu_war_news_117, 0x3895u, 0x184cu, 41u,
    "opeu_war_news_117" }, /* data/fnews */
  { opeu_war_news_118, 0x3895u, 0x1875u, 33u,
    "opeu_war_news_118" }, /* data/fnews */
  { opeu_war_news_119, 0x3895u, 0x1896u, 64u,
    "opeu_war_news_119" }, /* data/fnews */
  { opeu_war_news_120, 0x3895u, 0x18d6u, 63u,
    "opeu_war_news_120" }, /* data/fnews */
  { opeu_war_news_121, 0x3895u, 0x1915u, 80u,
    "opeu_war_news_121" }, /* data/fnews */
  { opeu_war_news_122, 0x3895u, 0x1965u, 63u,
    "opeu_war_news_122" }, /* data/fnews */
  { opeu_war_news_123, 0x3895u, 0x19a4u, 46u,
    "opeu_war_news_123" }, /* data/fnews */
  { opeu_war_news_124, 0x3895u, 0x19d2u, 37u,
    "opeu_war_news_124" }, /* data/fnews */
  { opeu_war_news_125, 0x3895u, 0x19f7u, 51u,
    "opeu_war_news_125" }, /* data/fnews */
  { opeu_war_news_126, 0x3895u, 0x1a2au, 65u,
    "opeu_war_news_126" }, /* data/fnews */
  { opeu_war_news_127, 0x3895u, 0x1a6bu, 44u,
    "opeu_war_news_127" }, /* data/fnews */
  { opeu_war_news_128, 0x3895u, 0x1a97u, 50u,
    "opeu_war_news_128" }, /* data/fnews */
  { opeu_war_news_129, 0x3895u, 0x1ac9u, 53u,
    "opeu_war_news_129" }, /* data/fnews */
  { opeu_war_news_130, 0x3895u, 0x1afeu, 82u,
    "opeu_war_news_130" }, /* data/fnews */
  { opeu_war_news_131, 0x3895u, 0x1b50u, 37u,
    "opeu_war_news_131" }, /* data/fnews */
  { opeu_war_news_132, 0x3895u, 0x1b75u, 36u,
    "opeu_war_news_132" }, /* data/fnews */
  { opeu_war_news_133, 0x3895u, 0x1b99u, 61u,
    "opeu_war_news_133" }, /* data/fnews */
  { opeu_war_news_134, 0x3895u, 0x1bd6u, 29u,
    "opeu_war_news_134" }, /* data/fnews */
  { opeu_war_news_135, 0x3895u, 0x1bf3u, 35u,
    "opeu_war_news_135" }, /* data/fnews */
  { opeu_war_news_136, 0x3895u, 0x1c16u, 28u,
    "opeu_war_news_136" }, /* data/fnews */
  { opeu_war_news_137, 0x3895u, 0x1c32u, 27u,
    "opeu_war_news_137" }, /* data/fnews */
  { opeu_war_news_138, 0x3895u, 0x1c4du, 39u,
    "opeu_war_news_138" }, /* data/fnews */
  { opeu_war_news_139, 0x3895u, 0x1c74u, 43u,
    "opeu_war_news_139" }, /* data/fnews */
  { opeu_war_news_140, 0x3895u, 0x1c9fu, 30u,
    "opeu_war_news_140" }, /* data/fnews */
  { opeu_war_news_141, 0x3895u, 0x1cbdu, 49u,
    "opeu_war_news_141" }, /* data/fnews */
  { opeu_weather_terrain_ratings, 0x3a64u, 0x0000u, 330u,
    "opeu_fg3a64_0000" }, /* data/fg3a64 */
  { opeu_air_sprite_save, 0x3a64u, 0x014au, 518u,
    "opeu_fg3a64_014a" }, /* data/fg3a64 */
  { opeu_bomb_sprite_save, 0x3a99u, 0x0000u, 512u,
    "opeu_fg3a99_0000" }, /* data/fg3a99 */
  { opeu_panel_border_tiles, 0x3ab9u, 0x0000u, 512u,
    "opeu_fg3ab9_0000" }, /* data/fg3ab9 */
  { opeu_advice_00, 0x3ad9u, 0x0000u, 37u,
    "opeu_advice_00" }, /* data/fadvice */
  { opeu_advice_01, 0x3ad9u, 0x0025u, 33u,
    "opeu_advice_01" }, /* data/fadvice */
  { opeu_advice_02, 0x3ad9u, 0x0046u, 75u,
    "opeu_advice_02" }, /* data/fadvice */
  { opeu_advice_03, 0x3ad9u, 0x0091u, 67u,
    "opeu_advice_03" }, /* data/fadvice */
  { opeu_advice_04, 0x3ad9u, 0x00d4u, 51u,
    "opeu_advice_04" }, /* data/fadvice */
  { opeu_advice_05, 0x3ad9u, 0x0107u, 40u,
    "opeu_advice_05" }, /* data/fadvice */
  { opeu_advice_06, 0x3ad9u, 0x012fu, 29u,
    "opeu_advice_06" }, /* data/fadvice */
  { opeu_advice_07, 0x3ad9u, 0x014cu, 35u,
    "opeu_advice_07" }, /* data/fadvice */
  { opeu_advice_08, 0x3ad9u, 0x016fu, 29u,
    "opeu_advice_08" }, /* data/fadvice */
  { opeu_advice_09, 0x3ad9u, 0x018cu, 59u,
    "opeu_advice_09" }, /* data/fadvice */
  { opeu_advice_10, 0x3ad9u, 0x01c7u, 73u,
    "opeu_advice_10" }, /* data/fadvice */
  { opeu_advice_11, 0x3ad9u, 0x0210u, 68u,
    "opeu_advice_11" }, /* data/fadvice */
  { opeu_advice_12, 0x3ad9u, 0x0254u, 36u,
    "opeu_advice_12" }, /* data/fadvice */
  { opeu_advice_13, 0x3ad9u, 0x0278u, 50u,
    "opeu_advice_13" }, /* data/fadvice */
  { opeu_advice_14, 0x3ad9u, 0x02aau, 60u,
    "opeu_advice_14" }, /* data/fadvice */
  { opeu_advice_15, 0x3ad9u, 0x02e6u, 49u,
    "opeu_advice_15" }, /* data/fadvice */
  { opeu_advice_16, 0x3ad9u, 0x0317u, 44u,
    "opeu_advice_16" }, /* data/fadvice */
  { opeu_advice_17, 0x3ad9u, 0x0343u, 39u,
    "opeu_advice_17" }, /* data/fadvice */
  { opeu_advice_18, 0x3ad9u, 0x036au, 44u,
    "opeu_advice_18" }, /* data/fadvice */
  { opeu_advice_19, 0x3ad9u, 0x0396u, 21u,
    "opeu_advice_19" }, /* data/fadvice */
  { opeu_advice_20, 0x3ad9u, 0x03abu, 51u,
    "opeu_advice_20" }, /* data/fadvice */
  { opeu_advice_21, 0x3ad9u, 0x03deu, 55u,
    "opeu_advice_21" }, /* data/fadvice */
  { opeu_advice_22, 0x3ad9u, 0x0415u, 40u,
    "opeu_advice_22" }, /* data/fadvice */
  { opeu_advice_23, 0x3ad9u, 0x043du, 38u,
    "opeu_advice_23" }, /* data/fadvice */
  { opeu_advice_24, 0x3ad9u, 0x0463u, 40u,
    "opeu_advice_24" }, /* data/fadvice */
  { opeu_advice_25, 0x3ad9u, 0x048bu, 19u,
    "opeu_advice_25" }, /* data/fadvice */
  { opeu_advice_26, 0x3ad9u, 0x049eu, 38u,
    "opeu_advice_26" }, /* data/fadvice */
  { opeu_advice_27, 0x3ad9u, 0x04c4u, 33u,
    "opeu_advice_27" }, /* data/fadvice */
  { opeu_advice_28, 0x3ad9u, 0x04e5u, 45u,
    "opeu_advice_28" }, /* data/fadvice */
  { opeu_advice_29, 0x3ad9u, 0x0512u, 30u,
    "opeu_advice_29" }, /* data/fadvice */
  { opeu_advice_30, 0x3ad9u, 0x0530u, 50u,
    "opeu_advice_30" }, /* data/fadvice */
  { opeu_advice_31, 0x3ad9u, 0x0562u, 48u,
    "opeu_advice_31" }, /* data/fadvice */
  { opeu_advice_32, 0x3ad9u, 0x0592u, 61u,
    "opeu_advice_32" }, /* data/fadvice */
  { opeu_advice_33, 0x3ad9u, 0x05cfu, 33u,
    "opeu_advice_33" }, /* data/fadvice */
  { opeu_advice_34, 0x3ad9u, 0x05f0u, 47u,
    "opeu_advice_34" }, /* data/fadvice */
  { opeu_advice_35, 0x3ad9u, 0x061fu, 60u,
    "opeu_advice_35" }, /* data/fadvice */
  { opeu_advice_36, 0x3ad9u, 0x065bu, 67u,
    "opeu_advice_36" }, /* data/fadvice */
  { opeu_advice_37, 0x3ad9u, 0x069eu, 41u,
    "opeu_advice_37" }, /* data/fadvice */
  { opeu_advice_38, 0x3ad9u, 0x06c7u, 27u,
    "opeu_advice_38" }, /* data/fadvice */
  { opeu_advice_39, 0x3ad9u, 0x06e2u, 36u,
    "opeu_advice_39" }, /* data/fadvice */
  { opeu_advice_40, 0x3ad9u, 0x0706u, 49u,
    "opeu_advice_40" }, /* data/fadvice */
  { opeu_advice_41, 0x3ad9u, 0x0737u, 55u,
    "opeu_advice_41" }, /* data/fadvice */
  { opeu_advice_42, 0x3ad9u, 0x076eu, 51u,
    "opeu_advice_42" }, /* data/fadvice */
  { opeu_advice_43, 0x3ad9u, 0x07a1u, 39u,
    "opeu_advice_43" }, /* data/fadvice */
  { opeu_advice_44, 0x3ad9u, 0x07c8u, 46u,
    "opeu_advice_44" }, /* data/fadvice */
  { opeu_advice_45, 0x3ad9u, 0x07f6u, 44u,
    "opeu_advice_45" }, /* data/fadvice */
  { opeu_advice_46, 0x3ad9u, 0x0822u, 49u,
    "opeu_advice_46" }, /* data/fadvice */
  { opeu_advice_47, 0x3ad9u, 0x0853u, 71u,
    "opeu_advice_47" }, /* data/fadvice */
  { opeu_advice_48, 0x3ad9u, 0x089au, 52u,
    "opeu_advice_48" }, /* data/fadvice */
  { opeu_advice_49, 0x3ad9u, 0x08ceu, 39u,
    "opeu_advice_49" }, /* data/fadvice */
  { opeu_advice_50, 0x3ad9u, 0x08f5u, 58u,
    "opeu_advice_50" }, /* data/fadvice */
  { opeu_advice_51, 0x3ad9u, 0x092fu, 49u,
    "opeu_advice_51" }, /* data/fadvice */
  { opeu_advice_52, 0x3ad9u, 0x0960u, 37u,
    "opeu_advice_52" }, /* data/fadvice */
  { opeu_advice_53, 0x3ad9u, 0x0985u, 51u,
    "opeu_advice_53" }, /* data/fadvice */
  { opeu_advice_54, 0x3ad9u, 0x09b8u, 36u,
    "opeu_advice_54" }, /* data/fadvice */
  { opeu_advice_55, 0x3ad9u, 0x09dcu, 66u,
    "opeu_advice_55" }, /* data/fadvice */
  { opeu_advice_56, 0x3ad9u, 0x0a1eu, 35u,
    "opeu_advice_56" }, /* data/fadvice */
  { opeu_advice_57, 0x3ad9u, 0x0a41u, 33u,
    "opeu_advice_57" }, /* data/fadvice */
  { opeu_advice_58, 0x3ad9u, 0x0a62u, 30u,
    "opeu_advice_58" }, /* data/fadvice */
  { opeu_advice_59, 0x3ad9u, 0x0a80u, 41u,
    "opeu_advice_59" }, /* data/fadvice */
  { opeu_advice_60, 0x3ad9u, 0x0aa9u, 29u,
    "opeu_advice_60" }, /* data/fadvice */
  { opeu_advice_61, 0x3ad9u, 0x0ac6u, 21u,
    "opeu_advice_61" }, /* data/fadvice */
  { opeu_advice_62, 0x3ad9u, 0x0adbu, 31u,
    "opeu_advice_62" }, /* data/fadvice */
  { opeu_advice_63, 0x3ad9u, 0x0afau, 76u,
    "opeu_advice_63" }, /* data/fadvice */
  { opeu_advice_64, 0x3ad9u, 0x0b46u, 51u,
    "opeu_advice_64" }, /* data/fadvice */
  { opeu_advice_65, 0x3ad9u, 0x0b79u, 70u,
    "opeu_advice_65" }, /* data/fadvice */
  { opeu_advice_66, 0x3ad9u, 0x0bbfu, 53u,
    "opeu_advice_66" }, /* data/fadvice */
  { opeu_advice_67, 0x3ad9u, 0x0bf4u, 79u,
    "opeu_advice_67" }, /* data/fadvice */
  { opeu_advice_68, 0x3ad9u, 0x0c43u, 71u,
    "opeu_advice_68" }, /* data/fadvice */
  { opeu_advice_69, 0x3ad9u, 0x0c8au, 40u,
    "opeu_advice_69" }, /* data/fadvice */
  { opeu_advice_70, 0x3ad9u, 0x0cb2u, 41u,
    "opeu_advice_70" }, /* data/fadvice */
  { opeu_advice_71, 0x3ad9u, 0x0cdbu, 55u,
    "opeu_advice_71" }, /* data/fadvice */
  { opeu_advice_72, 0x3ad9u, 0x0d12u, 86u,
    "opeu_advice_72" }, /* data/fadvice */
  { opeu_advice_73, 0x3ad9u, 0x0d68u, 69u,
    "opeu_advice_73" }, /* data/fadvice */
  { opeu_advice_74, 0x3ad9u, 0x0dadu, 89u,
    "opeu_advice_74" }, /* data/fadvice */
  { opeu_advice_75, 0x3ad9u, 0x0e06u, 50u,
    "opeu_advice_75" }, /* data/fadvice */
  { opeu_advice_76, 0x3ad9u, 0x0e38u, 85u,
    "opeu_advice_76" }, /* data/fadvice */
  { opeu_advice_77, 0x3ad9u, 0x0e8du, 54u,
    "opeu_advice_77" }, /* data/fadvice */
  { opeu_advice_78, 0x3ad9u, 0x0ec3u, 66u,
    "opeu_advice_78" }, /* data/fadvice */
  { opeu_advice_79, 0x3ad9u, 0x0f05u, 72u,
    "opeu_advice_79" }, /* data/fadvice */
  { opeu_advice_80, 0x3ad9u, 0x0f4du, 45u,
    "opeu_advice_80" }, /* data/fadvice */
  { opeu_advice_81, 0x3ad9u, 0x0f7au, 88u,
    "opeu_advice_81" }, /* data/fadvice */
  { opeu_advice_82, 0x3ad9u, 0x0fd2u, 73u,
    "opeu_advice_82" }, /* data/fadvice */
  { opeu_advice_83, 0x3ad9u, 0x101bu, 55u,
    "opeu_advice_83" }, /* data/fadvice */
  { opeu_advice_84, 0x3ad9u, 0x1052u, 77u,
    "opeu_advice_84" }, /* data/fadvice */
  { opeu_advice_85, 0x3ad9u, 0x109fu, 60u,
    "opeu_advice_85" }, /* data/fadvice */
  { opeu_advice_86, 0x3ad9u, 0x10dbu, 87u,
    "opeu_advice_86" }, /* data/fadvice */
  { opeu_advice_87, 0x3ad9u, 0x1132u, 53u,
    "opeu_advice_87" }, /* data/fadvice */
  { opeu_advice_88, 0x3ad9u, 0x1167u, 76u,
    "opeu_advice_88" }, /* data/fadvice */
  { opeu_advice_89, 0x3ad9u, 0x11b3u, 67u,
    "opeu_advice_89" }, /* data/fadvice */
  { opeu_advice_90, 0x3ad9u, 0x11f6u, 67u,
    "opeu_advice_90" }, /* data/fadvice */
  { opeu_advice_91, 0x3ad9u, 0x1239u, 64u,
    "opeu_advice_91" }, /* data/fadvice */
  { opeu_advice_92, 0x3ad9u, 0x1279u, 69u,
    "opeu_advice_92" }, /* data/fadvice */
  { opeu_reply_000, 0x3c05u, 0x0000u, 54u, "opeu_reply_000" }, /* data/freply */
  { opeu_reply_001, 0x3c05u, 0x0036u, 33u, "opeu_reply_001" }, /* data/freply */
  { opeu_reply_002, 0x3c05u, 0x0057u, 38u, "opeu_reply_002" }, /* data/freply */
  { opeu_reply_003, 0x3c05u, 0x007du, 26u, "opeu_reply_003" }, /* data/freply */
  { opeu_reply_004, 0x3c05u, 0x0097u, 56u, "opeu_reply_004" }, /* data/freply */
  { opeu_reply_005, 0x3c05u, 0x00cfu, 75u, "opeu_reply_005" }, /* data/freply */
  { opeu_reply_006, 0x3c05u, 0x011au, 23u, "opeu_reply_006" }, /* data/freply */
  { opeu_reply_007, 0x3c05u, 0x0131u, 53u, "opeu_reply_007" }, /* data/freply */
  { opeu_reply_008, 0x3c05u, 0x0166u, 53u, "opeu_reply_008" }, /* data/freply */
  { opeu_reply_009, 0x3c05u, 0x019bu, 49u, "opeu_reply_009" }, /* data/freply */
  { opeu_reply_010, 0x3c05u, 0x01ccu, 54u, "opeu_reply_010" }, /* data/freply */
  { opeu_reply_011, 0x3c05u, 0x0202u, 39u, "opeu_reply_011" }, /* data/freply */
  { opeu_reply_012, 0x3c05u, 0x0229u, 53u, "opeu_reply_012" }, /* data/freply */
  { opeu_reply_013, 0x3c05u, 0x025eu, 75u, "opeu_reply_013" }, /* data/freply */
  { opeu_reply_014, 0x3c05u, 0x02a9u, 61u, "opeu_reply_014" }, /* data/freply */
  { opeu_reply_015, 0x3c05u, 0x02e6u, 73u, "opeu_reply_015" }, /* data/freply */
  { opeu_reply_016, 0x3c05u, 0x032fu, 46u, "opeu_reply_016" }, /* data/freply */
  { opeu_reply_017, 0x3c05u, 0x035du, 43u, "opeu_reply_017" }, /* data/freply */
  { opeu_reply_018, 0x3c05u, 0x0388u, 51u, "opeu_reply_018" }, /* data/freply */
  { opeu_reply_019, 0x3c05u, 0x03bbu, 27u, "opeu_reply_019" }, /* data/freply */
  { opeu_reply_020, 0x3c05u, 0x03d6u, 71u, "opeu_reply_020" }, /* data/freply */
  { opeu_reply_021, 0x3c05u, 0x041du, 1u, "opeu_reply_021" },  /* data/freply */
  { opeu_reply_022, 0x3c05u, 0x041eu, 54u, "opeu_reply_022" }, /* data/freply */
  { opeu_reply_023, 0x3c05u, 0x0454u, 43u, "opeu_reply_023" }, /* data/freply */
  { opeu_reply_024, 0x3c05u, 0x047fu, 59u, "opeu_reply_024" }, /* data/freply */
  { opeu_reply_025, 0x3c05u, 0x04bau, 49u, "opeu_reply_025" }, /* data/freply */
  { opeu_reply_026, 0x3c05u, 0x04ebu, 73u, "opeu_reply_026" }, /* data/freply */
  { opeu_reply_027, 0x3c05u, 0x0534u, 36u, "opeu_reply_027" }, /* data/freply */
  { opeu_reply_028, 0x3c05u, 0x0558u, 37u, "opeu_reply_028" }, /* data/freply */
  { opeu_reply_029, 0x3c05u, 0x057du, 37u, "opeu_reply_029" }, /* data/freply */
  { opeu_reply_030, 0x3c05u, 0x05a2u, 36u, "opeu_reply_030" }, /* data/freply */
  { opeu_reply_031, 0x3c05u, 0x05c6u, 41u, "opeu_reply_031" }, /* data/freply */
  { opeu_reply_032, 0x3c05u, 0x05efu, 50u, "opeu_reply_032" }, /* data/freply */
  { opeu_reply_033, 0x3c05u, 0x0621u, 24u, "opeu_reply_033" }, /* data/freply */
  { opeu_reply_034, 0x3c05u, 0x0639u, 32u, "opeu_reply_034" }, /* data/freply */
  { opeu_reply_035, 0x3c05u, 0x0659u, 32u, "opeu_reply_035" }, /* data/freply */
  { opeu_reply_036, 0x3c05u, 0x0679u, 51u, "opeu_reply_036" }, /* data/freply */
  { opeu_reply_037, 0x3c05u, 0x06acu, 57u, "opeu_reply_037" }, /* data/freply */
  { opeu_reply_038, 0x3c05u, 0x06e5u, 49u, "opeu_reply_038" }, /* data/freply */
  { opeu_reply_039, 0x3c05u, 0x0716u, 45u, "opeu_reply_039" }, /* data/freply */
  { opeu_reply_040, 0x3c05u, 0x0743u, 67u, "opeu_reply_040" }, /* data/freply */
  { opeu_reply_041, 0x3c05u, 0x0786u, 37u, "opeu_reply_041" }, /* data/freply */
  { opeu_reply_042, 0x3c05u, 0x07abu, 50u, "opeu_reply_042" }, /* data/freply */
  { opeu_reply_043, 0x3c05u, 0x07ddu, 25u, "opeu_reply_043" }, /* data/freply */
  { opeu_reply_044, 0x3c05u, 0x07f6u, 44u, "opeu_reply_044" }, /* data/freply */
  { opeu_reply_045, 0x3c05u, 0x0822u, 75u, "opeu_reply_045" }, /* data/freply */
  { opeu_reply_046, 0x3c05u, 0x086du, 72u, "opeu_reply_046" }, /* data/freply */
  { opeu_reply_047, 0x3c05u, 0x08b5u, 56u, "opeu_reply_047" }, /* data/freply */
  { opeu_reply_048, 0x3c05u, 0x08edu, 55u, "opeu_reply_048" }, /* data/freply */
  { opeu_reply_049, 0x3c05u, 0x0924u, 24u, "opeu_reply_049" }, /* data/freply */
  { opeu_reply_050, 0x3c05u, 0x093cu, 84u, "opeu_reply_050" }, /* data/freply */
  { opeu_reply_051, 0x3c05u, 0x0990u, 50u, "opeu_reply_051" }, /* data/freply */
  { opeu_reply_052, 0x3c05u, 0x09c2u, 54u, "opeu_reply_052" }, /* data/freply */
  { opeu_reply_053, 0x3c05u, 0x09f8u, 59u, "opeu_reply_053" }, /* data/freply */
  { opeu_reply_054, 0x3c05u, 0x0a33u, 46u, "opeu_reply_054" }, /* data/freply */
  { opeu_reply_055, 0x3c05u, 0x0a61u, 54u, "opeu_reply_055" }, /* data/freply */
  { opeu_reply_056, 0x3c05u, 0x0a97u, 83u, "opeu_reply_056" }, /* data/freply */
  { opeu_reply_057, 0x3c05u, 0x0aeau, 18u, "opeu_reply_057" }, /* data/freply */
  { opeu_reply_058, 0x3c05u, 0x0afcu, 59u, "opeu_reply_058" }, /* data/freply */
  { opeu_reply_059, 0x3c05u, 0x0b37u, 28u, "opeu_reply_059" }, /* data/freply */
  { opeu_reply_060, 0x3c05u, 0x0b53u, 21u, "opeu_reply_060" }, /* data/freply */
  { opeu_reply_061, 0x3c05u, 0x0b68u, 54u, "opeu_reply_061" }, /* data/freply */
  { opeu_reply_062, 0x3c05u, 0x0b9eu, 53u, "opeu_reply_062" }, /* data/freply */
  { opeu_reply_063, 0x3c05u, 0x0bd3u, 60u, "opeu_reply_063" }, /* data/freply */
  { opeu_reply_064, 0x3c05u, 0x0c0fu, 65u, "opeu_reply_064" }, /* data/freply */
  { opeu_reply_065, 0x3c05u, 0x0c50u, 61u, "opeu_reply_065" }, /* data/freply */
  { opeu_reply_066, 0x3c05u, 0x0c8du, 70u, "opeu_reply_066" }, /* data/freply */
  { opeu_reply_067, 0x3c05u, 0x0cd3u, 62u, "opeu_reply_067" }, /* data/freply */
  { opeu_reply_068, 0x3c05u, 0x0d11u, 46u, "opeu_reply_068" }, /* data/freply */
  { opeu_reply_069, 0x3c05u, 0x0d3fu, 44u, "opeu_reply_069" }, /* data/freply */
  { opeu_reply_070, 0x3c05u, 0x0d6bu, 72u, "opeu_reply_070" }, /* data/freply */
  { opeu_reply_071, 0x3c05u, 0x0db3u, 67u, "opeu_reply_071" }, /* data/freply */
  { opeu_reply_072, 0x3c05u, 0x0df6u, 33u, "opeu_reply_072" }, /* data/freply */
  { opeu_reply_073, 0x3c05u, 0x0e17u, 41u, "opeu_reply_073" }, /* data/freply */
  { opeu_reply_074, 0x3c05u, 0x0e40u, 48u, "opeu_reply_074" }, /* data/freply */
  { opeu_reply_075, 0x3c05u, 0x0e70u, 36u, "opeu_reply_075" }, /* data/freply */
  { opeu_reply_076, 0x3c05u, 0x0e94u, 58u, "opeu_reply_076" }, /* data/freply */
  { opeu_reply_077, 0x3c05u, 0x0eceu, 53u, "opeu_reply_077" }, /* data/freply */
  { opeu_reply_078, 0x3c05u, 0x0f03u, 53u, "opeu_reply_078" }, /* data/freply */
  { opeu_reply_079, 0x3c05u, 0x0f38u, 54u, "opeu_reply_079" }, /* data/freply */
  { opeu_reply_080, 0x3c05u, 0x0f6eu, 70u, "opeu_reply_080" }, /* data/freply */
  { opeu_reply_081, 0x3c05u, 0x0fb4u, 86u, "opeu_reply_081" }, /* data/freply */
  { opeu_reply_082, 0x3c05u, 0x100au, 54u, "opeu_reply_082" }, /* data/freply */
  { opeu_reply_083, 0x3c05u, 0x1040u, 56u, "opeu_reply_083" }, /* data/freply */
  { opeu_reply_084, 0x3c05u, 0x1078u, 42u, "opeu_reply_084" }, /* data/freply */
  { opeu_reply_085, 0x3c05u, 0x10a2u, 85u, "opeu_reply_085" }, /* data/freply */
  { opeu_reply_086, 0x3c05u, 0x10f7u, 82u, "opeu_reply_086" }, /* data/freply */
  { opeu_reply_087, 0x3c05u, 0x1149u, 49u, "opeu_reply_087" }, /* data/freply */
  { opeu_reply_088, 0x3c05u, 0x117au, 69u, "opeu_reply_088" }, /* data/freply */
  { opeu_reply_089, 0x3c05u, 0x11bfu, 83u, "opeu_reply_089" }, /* data/freply */
  { opeu_reply_090, 0x3c05u, 0x1212u, 66u, "opeu_reply_090" }, /* data/freply */
  { opeu_reply_091, 0x3c05u, 0x1254u, 82u, "opeu_reply_091" }, /* data/freply */
  { opeu_reply_092, 0x3c05u, 0x12a6u, 69u, "opeu_reply_092" }, /* data/freply */
  { opeu_reply_093, 0x3c05u, 0x12ebu, 79u, "opeu_reply_093" }, /* data/freply */
  { opeu_reply_094, 0x3c05u, 0x133au, 54u, "opeu_reply_094" }, /* data/freply */
  { opeu_reply_095, 0x3c05u, 0x1370u, 45u, "opeu_reply_095" }, /* data/freply */
  { opeu_reply_096, 0x3c05u, 0x139du, 31u, "opeu_reply_096" }, /* data/freply */
  { opeu_reply_097, 0x3c05u, 0x13bcu, 43u, "opeu_reply_097" }, /* data/freply */
  { opeu_reply_098, 0x3c05u, 0x13e7u, 66u, "opeu_reply_098" }, /* data/freply */
  { opeu_reply_099, 0x3c05u, 0x1429u, 19u, "opeu_reply_099" }, /* data/freply */
  { opeu_reply_100, 0x3c05u, 0x143cu, 48u, "opeu_reply_100" }, /* data/freply */
  { opeu_reply_101, 0x3c05u, 0x146cu, 31u, "opeu_reply_101" }, /* data/freply */
  { opeu_reply_102, 0x3c05u, 0x148bu, 33u, "opeu_reply_102" }, /* data/freply */
  { opeu_reply_103, 0x3c05u, 0x14acu, 28u, "opeu_reply_103" }, /* data/freply */
  { opeu_reply_104, 0x3c05u, 0x14c8u, 40u, "opeu_reply_104" }, /* data/freply */
  { opeu_reply_105, 0x3c05u, 0x14f0u, 58u, "opeu_reply_105" }, /* data/freply */
  { opeu_reply_106, 0x3c05u, 0x152au, 24u, "opeu_reply_106" }, /* data/freply */
  { opeu_reply_107, 0x3c05u, 0x1542u, 44u, "opeu_reply_107" }, /* data/freply */
  { opeu_reply_108, 0x3c05u, 0x156eu, 76u, "opeu_reply_108" }, /* data/freply */
  { opeu_reply_109, 0x3c05u, 0x15bau, 36u, "opeu_reply_109" }, /* data/freply */
  { opeu_reply_110, 0x3c05u, 0x15deu, 41u, "opeu_reply_110" }, /* data/freply */
  { opeu_reply_111, 0x3c05u, 0x1607u, 46u, "opeu_reply_111" }, /* data/freply */
  { opeu_reply_112, 0x3c05u, 0x1635u, 43u, "opeu_reply_112" }, /* data/freply */
  { opeu_reply_113, 0x3c05u, 0x1660u, 41u, "opeu_reply_113" }, /* data/freply */
  { opeu_reply_114, 0x3c05u, 0x1689u, 44u, "opeu_reply_114" }, /* data/freply */
  { opeu_reply_115, 0x3c05u, 0x16b5u, 75u, "opeu_reply_115" }, /* data/freply */
  { opeu_reply_116, 0x3c05u, 0x1700u, 54u, "opeu_reply_116" }, /* data/freply */
  { opeu_reply_117, 0x3c05u, 0x1736u, 35u, "opeu_reply_117" }, /* data/freply */
  { opeu_reply_118, 0x3c05u, 0x1759u, 29u, "opeu_reply_118" }, /* data/freply */
  { opeu_reply_119, 0x3c05u, 0x1776u, 41u, "opeu_reply_119" }, /* data/freply */
  { opeu_reply_120, 0x3c05u, 0x179fu, 73u, "opeu_reply_120" }, /* data/freply */
  { opeu_reply_121, 0x3c05u, 0x17e8u, 47u, "opeu_reply_121" }, /* data/freply */
  { opeu_reply_122, 0x3c05u, 0x1817u, 63u, "opeu_reply_122" }, /* data/freply */
  { opeu_reply_123, 0x3c05u, 0x1856u, 52u, "opeu_reply_123" }, /* data/freply */
  { opeu_reply_124, 0x3c05u, 0x188au, 48u, "opeu_reply_124" }, /* data/freply */
  { opeu_music_tracks, 0x3d91u, 0x0000u, 190u,
    "opeu_music_tracks" }, /* data/fmusic */
  { opeu_music_voices, 0x3d91u, 0x00beu, 1629u,
    "opeu_music_voices" }, /* data/fmusic */
  { opeu_music_events, 0x3d91u, 0x071bu, 29855u,
    "opeu_music_events" }, /* data/fmusic */
  { opeu_sound_effects, 0x3d91u, 0x7bbau, 847u,
    "opeu_sound_effects" }, /* data/fmusic */
  { opeu_path_cell_queue, 0x4582u, 0x0000u, 3200u,
    "opeu_fg4582_0000" }, /* data/fg4582 */
  { opeu_path_cost_grid, 0x4582u, 0x0c80u, 3200u,
    "opeu_fg4582_0c80" }, /* data/fg4582 */
  { opeu_turn_order, 0x4712u, 0x2012u, 64u,
    "opeu_turn_order" }, /* data/mapstate */
  { opeu_objective_links, 0x4712u, 0x2052u, 84u,
    "opeu_objective_links" }, /* data/mapstate */
  { opeu_unit_links, 0x4712u, 0x20a6u, 16u,
    "opeu_unit_links" }, /* data/mapstate */
  { opeu_map_cells, 0x4712u, 0x20b6u, 1600u,
    "opeu_map_cells" }, /* data/mapstate */
  { opeu_influence_map, 0x4712u, 0x26f6u, 200u,
    "opeu_influence_map" }, /* data/mapstate */
  { opeu_unit_type_names, 0x4712u, 0x27beu, 1314u,
    "opeu_unit_type_names" }, /* data/dgnames */
  { opeu_general_names, 0x4712u, 0x2ce0u, 428u,
    "opeu_general_names" }, /* data/dgnames */
  { opeu_resource_filenames, 0x4712u, 0x2e8cu, 226u,
    "opeu_resource_filenames" }, /* data/dgnames */
  { opeu_turn_variables, 0x4712u, 0x2f6eu, 450u,
    "opeu_turn_variables" }, /* data/dgnames */
  { opeu_request_capacities, 0x4712u, 0x3130u, 16u,
    "opeu_request_capacities" }, /* data/dgnames */
  { opeu_capacity_a_weights, 0x4712u, 0x3140u, 16u,
    "opeu_capacity_a_weights" }, /* data/dgnames */
  { opeu_capacity_c_weights, 0x4712u, 0x3150u, 16u,
    "opeu_capacity_c_weights" }, /* data/dgnames */
  { opeu_person_templates, 0x4712u, 0x3160u, 372u,
    "opeu_person_templates" }, /* data/dgnames */
  { opeu_template_tables, 0x4712u, 0x32d4u, 94u,
    "opeu_template_tables" }, /* data/dgnames */
  { opeu_element_types, 0x4712u, 0x3332u, 2054u,
    "opeu_element_types" }, /* data/dgnames */
  { opeu_weather_names, 0x4712u, 0x3b38u, 12u,
    "opeu_weather_names" }, /* data/dgnames */
  { opeu_side_names, 0x4712u, 0x3b44u, 4u,
    "opeu_side_names" }, /* data/dgnames */
  { opeu_mask_names, 0x4712u, 0x3b48u, 12u,
    "opeu_mask_names" }, /* data/dgnames */
  { opeu_role_names, 0x4712u, 0x3b54u, 14u,
    "opeu_role_names" }, /* data/dgnames */
  { opeu_entry_names, 0x4712u, 0x3b62u, 16u,
    "opeu_entry_names" }, /* data/dgnames */
  { opeu_terrain_names, 0x4712u, 0x3b72u, 22u,
    "opeu_terrain_names" }, /* data/dgnames */
  { opeu_order_names, 0x4712u, 0x3b88u, 34u,
    "opeu_order_names" }, /* data/dgnames */
  { opeu_supply_names, 0x4712u, 0x3baau, 6u,
    "opeu_supply_names" }, /* data/dgnames */
  { opeu_small_glyphs, 0x4712u, 0x3bb0u, 702u,
    "opeu_small_glyphs" }, /* data/dgnames */
  { opeu_text_vars, 0x4712u, 0x3ebau, 50u, "opeu_text_vars" }, /* data/dgtext */
  { opeu_text_font, 0x4712u, 0x3eecu, 1536u,
    "opeu_text_font" }, /* data/dgfont */
  { opeu_setup_text, 0x4712u, 0x44ecu, 3211u,
    "opeu_setup_text" }, /* data/dgsetup */
  { opeu_startup_text, 0x4712u, 0x5184u, 488u,
    "opeu_startup_text" }, /* data/dgstart */
  { opeu_label_text, 0x4712u, 0x537cu, 2102u,
    "opeu_label_text" }, /* data/dglabels */
  { opeu_briefing_text, 0x4712u, 0x5bb2u, 6056u,
    "opeu_briefing_text" }, /* data/dgbrief */
  { opeu_side_initializers, 0x4712u, 0x735au, 24u,
    "opeu_side_initializers" }, /* data/dgevents */
  { opeu_event_tables, 0x4712u, 0x7372u, 2742u,
    "opeu_event_tables" }, /* data/dgevents */
  { opeu_request_supplies_name, 0x4712u, 0x7e28u, 9u,
    "opeu_request_supplies_name" }, /* data/uitext */
  { opeu_request_battalions_name, 0x4712u, 0x7e31u, 11u,
    "opeu_request_battalions_name" }, /* data/uitext */
  { opeu_request_regiments_name, 0x4712u, 0x7e3cu, 10u,
    "opeu_request_regiments_name" }, /* data/uitext */
  { opeu_request_kind_names, 0x4712u, 0x7e46u, 6u,
    "opeu_request_kind_names" }, /* data/uitext */
  { opeu_request_delay_bases, 0x4712u, 0x7e4cu, 3u,
    "opeu_request_delay_bases" }, /* data/uitext */
  { opeu_equipment_delivery_format, 0x4712u, 0x7e4fu, 13u,
    "opeu_equipment_delivery_format" }, /* data/uitext */
  { opeu_request_weights2, 0x4712u, 0x7e5cu, 6u,
    "opeu_request_weights2" }, /* data/uitext */
  { opeu_context_name_format, 0x4712u, 0x7e62u, 23u,
    "opeu_context_name_format" }, /* data/uitext */
  { opeu_context_stats_format, 0x4712u, 0x7e79u, 146u,
    "opeu_context_stats_format" }, /* data/uitext */
  { opeu_context_blank_format, 0x4712u, 0x7f0bu, 16u,
    "opeu_context_blank_format" }, /* data/uitext */
  { opeu_context_text_padding, 0x4712u, 0x7f1bu, 1u,
    "opeu_context_text_padding" }, /* data/uitext */
  { opeu_scenario_contexts, 0x4712u, 0x7f1cu, 6u,
    "opeu_scenario_contexts" }, /* data/uitext */
  { opeu_menu_pass_label, 0x4712u, 0x7f22u, 7u,
    "opeu_menu_pass_label" }, /* data/menutext */
  { opeu_menu_tact_label, 0x4712u, 0x7f29u, 7u,
    "opeu_menu_tact_label" }, /* data/menutext */
  { opeu_menu_city_label, 0x4712u, 0x7f30u, 7u,
    "opeu_menu_city_label" }, /* data/menutext */
  { opeu_menu_tsfr_label, 0x4712u, 0x7f37u, 6u,
    "opeu_menu_tsfr_label" }, /* data/menutext */
  { opeu_menu_ofcr_label, 0x4712u, 0x7f3du, 7u,
    "opeu_menu_ofcr_label" }, /* data/menutext */
  { opeu_menu_info_label, 0x4712u, 0x7f44u, 7u,
    "opeu_menu_info_label" }, /* data/menutext */
  { opeu_menu_orgz_label, 0x4712u, 0x7f4bu, 7u,
    "opeu_menu_orgz_label" }, /* data/menutext */
  { opeu_menu_rqst_label, 0x4712u, 0x7f52u, 7u,
    "opeu_menu_rqst_label" }, /* data/menutext */
  { opeu_menu_air_label, 0x4712u, 0x7f59u, 6u,
    "opeu_menu_air_label" }, /* data/menutext */
  { opeu_menu_spfc_label, 0x4712u, 0x7f5fu, 7u,
    "opeu_menu_spfc_label" }, /* data/menutext */
  { opeu_menu_game_label, 0x4712u, 0x7f66u, 7u,
    "opeu_menu_game_label" }, /* data/menutext */
  { opeu_control_prompt, 0x4712u, 0x7f6du, 31u,
    "opeu_control_prompt" }, /* data/ctrltext */
  { opeu_control_ready, 0x4712u, 0x7f8cu, 39u,
    "opeu_control_ready" }, /* data/ctrltext */
  { opeu_control_text_padding, 0x4712u, 0x7fb3u, 1u,
    "opeu_control_text_padding" }, /* data/ctrltext */
  { opeu_control_actions, 0x4712u, 0x7fb4u, 44u,
    "opeu_control_actions" }, /* data/ctrltext */
  { opeu_control_menu, 0x4712u, 0x7fe0u, 144u,
    "opeu_control_menu" }, /* data/menutext */
  { opeu_turn_text, 0x4712u, 0x8070u, 798u,
    "opeu_turn_text" }, /* data/dgturn */
  { opeu_order_text, 0x4712u, 0x8396u, 3452u,
    "opeu_order_text" }, /* data/dgorders */
  { opeu_supply_text, 0x4712u, 0x9112u, 3326u,
    "opeu_supply_text" }, /* data/dgsupply */
  { opeu_air_text, 0x4712u, 0x9e10u, 4126u, "opeu_air_text" }, /* data/dgair */
  { opeu_combat_text, 0x4712u, 0xb242u, 146u,
    "opeu_combat_text" }, /* data/dgcombat */
  { opeu_battle_text, 0x4712u, 0xb34eu, 3842u,
    "opeu_battle_text" }, /* data/dgbattle */
};
const unsigned port_image_object_count = std::size(port_image_objects);
