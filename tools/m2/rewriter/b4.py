"""The B4 pass (#13): far code pointers become enums.

Every code `FarPointer` slot of the data segment (the 23 tables, `NoonEvent::action`, `ListWindow::draw/select`) and
the presenter a daily show is handed become an `enum class <Name> : std::uint32_t` whose enumerators carry the routine's
original far address, `segment << 16 | offset`: the 4-byte slot keeps its bytes, so the data segment image is identical
through M2 (M4 renumbers the enums densely). The pass

  - declares the slot enums in common/data-segment.hpp (after `FarPointer`, re-exported beside it) and retypes the
    slots, and writes common/presenter.hpp for the presenters;
  - rewrites the data initialisers of game/data: each `{ offset, segment }` pair of a code slot becomes its enumerator,
    and the mission handlers' bytes become enumerators;
  - checks every enumerator against the routine link table it replaces (the routine at segment * 16 + offset).

The dispatch sites (`port_call(seg * 16 + off)`) and the run-time writers are rewritten by hand: each has its own
handler signature, return value and contract (docs/port.md "M2 rewriter")."""
from __future__ import annotations

import re
import textwrap
from dataclasses import dataclass, field
from pathlib import Path, PurePosixPath

from .source import SOURCES, SourceTree

SEGMENT_HEADER = PurePosixPath("game/common/data-segment.hpp")
PRESENTER_HEADER = PurePosixPath("game/common/presenter.hpp")
LINK_TABLE = PurePosixPath("game/link/gen/link-table.cpp")


@dataclass(frozen=True)
class Enumerator:
  name:    str
  segment: int
  offset:  int
  routine: str | None  # the routine the link table holds at segment * 16 + offset; None: 0:0 or not a linked routine
  comment: str = ""

  @property
  def value(self) -> int:
    return self.segment << 16 | self.offset


@dataclass(frozen=True)
class CodeEnum:
  name:        str
  fields:      tuple[str, ...]  # the data-segment slots it types (empty: not a slot)
  comment:     str
  enumerators: tuple[Enumerator, ...]

  def ByPair(self) -> dict[tuple[int, int], Enumerator]:
    return {(e.offset, e.segment): e for e in self.enumerators}


def E(name: str, segment: int, offset: int, routine: str | None, comment: str = "") -> Enumerator:
  return Enumerator(name, segment, offset, routine, comment)


VALUE_NOTE = ("A code enumerator's value is its routine's original far address, segment << 16 | offset, so the 4-byte slot keeps "
              "the bytes it had and the data segment image stays identical through M2; M4 renumbers the enums densely.")

SLOT_ENUMS: tuple[CodeEnum, ...] = (
  CodeEnum("NoonAction", ("action",), "What a noon event does besides its news (NoonEvent::action; core/noonevt). " + VALUE_NOTE, (
    E("NONE", 0x0000, 0x0000, None, "no action"),
    E("NOTHING", 0x0e57, 0x0004, None, "an action that does nothing"),
    E("BLOCK_AXIS_REGIMENTS", 0x0e57, 0x0006, None),
    E("BLOCK_AXIS_SUPPLIES_AND_BATTALIONS", 0x0e57, 0x000e, None),
    E("BLOCK_AXIS_BATTALIONS_AND_REGIMENTS", 0x0e57, 0x0018, None),
    E("ALLOW_AXIS_BATTALIONS_AND_REGIMENTS", 0x0e57, 0x0022, None),
    E("RAISE_AXIS_MORALE", 0x0e57, 0x002c, None),
    E("LOWER_AXIS_MORALE", 0x0e57, 0x0066, None),
    E("LOWER_ALLIED_MORALE", 0x0e57, 0x009c, None),
    E("AXIS_MORALE_BLOCKADE", 0x0e57, 0x00d2, None),
    E("AXIS_RESERVES", 0x0e57, 0x00dc, None, "the Axis expects its reserves"),
    E("ALLIED_RESERVES", 0x0e57, 0x010e, None, "the Allies expect their reserves"),
    E("ALLIED_POOL", 0x0e57, 0x0140, None, "the Allied pool gains 500"),
  )),
  CodeEnum("ListDraw", ("draw",), "The row-draw routine of a list window (ListWindow::draw; io/listui).", (
    E("GENERALS", 0x06db, 0x121e, "opeu_general_list_draw_rows"),
    E("GROUPS", 0x0c45, 0x0002, "opeu_tbl_draw_groups"),
    E("MISSIONS", 0x0c45, 0x01e6, "opeu_tbl_draw_missions"),
    E("UNITS", 0x0c45, 0x03a6, "opeu_tbl_draw_units"),
    E("ORDERS", 0x0c45, 0x0650, "opeu_tbl_draw_orders"),
    E("ELEMENTS", 0x0c45, 0x07ac, "opeu_tbl_draw_elements"),
    E("STAFF", 0x0c45, 0x092c, "opeu_tbl_draw_staff"),
    E("TYPES", 0x0c45, 0x0b0c, "opeu_tbl_draw_types"),
    E("SUPPLIES", 0x0c45, 0x0cc0, "opeu_tbl_draw_supplies"),
    E("STRENGTH", 0x0c45, 0x0e74, "opeu_tbl_draw_strength"),
    E("SAVES", 0x0c45, 0x0ff4, "opeu_tbl_draw_saves"),
    E("SUPPLY_TYPES", 0x20b3, 0x0dfe, "opeu_supply_draw_types"),
    E("SUPPLY_SLOTS", 0x20b3, 0x0fac, "opeu_supply_draw_slots"),
    E("REQUEST_TYPES", 0x22f9, 0x01f6, "opeu_rq_draw_types"),
    E("REQUEST_UNITS", 0x22f9, 0x04d2, "opeu_rq_draw_units"),
    E("DEPLOYMENT", 0x2af1, 0x0006, "opeu_deploy_draw_rows"),
  )),
  CodeEnum("ListSelect", ("select",), "The select routine of a list window (ListWindow::select; io/listdlg): which rows may be chosen, "
           "or what choosing one does.", (
    E("NONE", 0x0000, 0x0000, None, "not set: the window is not run as a dialog"),
    E("REROLL_STAFF", 0x06db, 0x0d28, "opeu_staff_reroll_selected"),
    E("ASSIGN_GENERAL", 0x06db, 0x0f3e, "opeu_general_assign_choice"),
    E("SAVE_ACCEPT", 0x0c45, 0x0ff0, "opeu_tbl_save_accept"),
    E("GROUP_TARGET", 0x1c23, 0x0006, "opeu_group_target_enabled"),
    E("ANY_ORDER_TARGET", 0x1c23, 0x0040, "opeu_menu_always"),
    E("SUPPLY_UNIT", 0x1c23, 0x021c, "opeu_supply_unit_enabled"),
    E("STOCK_CITY", 0x1d1e, 0x004e, "opeu_stock_city_enabled"),
    E("TRANSFER_GROUP", 0x1d42, 0x009a, "opeu_transfer_group_enabled"),
    E("TRANSFER_UNIT", 0x1d42, 0x00d4, "opeu_transfer_unit_enabled"),
    E("TRANSFER_ELEMENT", 0x1d42, 0x0110, "opeu_transfer_element_enabled"),
    E("ANY_GENERAL", 0x1d82, 0x000c, "opeu_commander_always"),
    E("DIVISION_TOGGLE", 0x1d82, 0x003c, "opeu_division_toggle"),
    E("VIEW_OBJECTIVE", 0x1fe6, 0x0120, "opeu_view_objective_ok", "own objectives that may be listed, enemy ones that are marked"),
    E("VIEW_NONE", 0x1fe6, 0x016e, "opeu_view_false", "no row"),
    E("VIEW_DIVISION", 0x1fe6, 0x0172, "opeu_view_group_ok", "a present division whose selected regiment may be selected"),
    E("VIEW_REGIMENT", 0x1fe6, 0x01ac, "opeu_view_unit_ok", "a present regiment that may be selected if ours, sighted if the enemy's"),
    E("VIEW_NONE_2", 0x1fe6, 0x0200, "opeu_view_false_2", "no row"),
    E("VIEW_NONE_3", 0x1fe6, 0x0204, "opeu_view_false_3", "no row"),
    E("VIEW_ANY", 0x1fe6, 0x0208, "opeu_view_true", "every row"),
    E("SUPPLY_UNIT_ELIGIBLE", 0x20b3, 0x0d40, "opeu_supply_unit_eligible"),
    E("ANY_SUPPLY_TYPE", 0x20b3, 0x0dfa, "opeu_supply_true"),
    E("RETURN_SLOT", 0x20b3, 0x0ed8, "opeu_supply_return_slot"),
    E("TALLY_MARK", 0x20b3, 0x10b8, "opeu_tally_mark"),
    E("REQUEST_TYPE", 0x22f9, 0x01f2, "opeu_rq_type_selectable"),
    E("REQUEST_UNIT", 0x22f9, 0x04ce, "opeu_rq_unit_selectable"),
    E("AIR_MARK_REGIMENT", 0x23b2, 0x06ca, "opeu_air_mark_regiment"),
    E("AIR_ASSIGN_TROOPS", 0x23b2, 0x06de, "opeu_air_assign_troops"),
    E("AIR_MARK_BATTALION", 0x23b2, 0x077a, "opeu_air_mark_battalion"),
    E("ANY_DEPLOYMENT", 0x2af1, 0x0002, "opeu_deploy_true"),
  )),
  CodeEnum("IdleHook", ("idle_hook",), "What runs while the keyboard is polled (idle_hook; io/keyboard).", (
    E("NONE", 0x0000, 0x0000, None),
    E("TEXT_CURSOR", 0x0541, 0x002e, None, "blinks the text cursor"),
  )),
  CodeEnum("MessageHook", ("message_hook",), "What runs while a dialog message shows (message_hook; core/scenbrf sets it, nothing calls it).", (
    E("NONE", 0x0000, 0x0000, None),
    E("BRIEFING_BLINK", 0x0d9b, 0x0000, "opeu_brief_blink", "blinks the briefing's named city"),
  )),
  CodeEnum("GlyphDraw", ("glyph_draw",), "How a console glyph is drawn (glyph_draw; core/textout).", (
    E("TRANSPARENT", 0x0000, 0x28a8, None),
    E("OPAQUE", 0x0000, 0x28c5, None),
  )),
  CodeEnum("SideInitializer", ("side_initializers",), "The briefing that opens a side's first turn, by scenario (side_initializers; core/sideinit).", (
    E("FRANCE", 0x0d9b, 0x0038, "opeu_brief_france"),
    E("AFRICA", 0x0d9b, 0x0180, "opeu_brief_africa"),
    E("KURSK", 0x0d9b, 0x0300, "opeu_brief_kursk"),
    E("NORMANDY", 0x0d9b, 0x0480, "opeu_brief_normandy"),
    E("ARDENNES", 0x0d9b, 0x061e, "opeu_brief_ardennes"),
    E("BERLIN", 0x0d9b, 0x0784, "opeu_brief_berlin"),
  )),
  CodeEnum("MissionHandler", ("mission_handlers",), "What a special-forces mission does when it comes due, by MissionKind (mission_handlers; core/msntick).", (
    E("RAID_CITY", 0x0e8e, 0x028c, "opeu_mission_raid_objective"),
    E("RAID_UNIT", 0x0e8e, 0x036a, "opeu_mission_raid_unit"),
    E("SABOTAGE", 0x0e8e, 0x044c, "opeu_mission_sabotage"),
    E("CUT_LINES_CITY", 0x0e8e, 0x052c, "opeu_mission_objective_3"),
    E("CUT_LINES_UNIT", 0x0e8e, 0x0614, "opeu_mission_unit_4"),
    E("ASSASSINATION", 0x0e8e, 0x06f8, "opeu_mission_assassinate"),
    E("RECON_CITY", 0x0e8e, 0x0910, "opeu_mission_recon_objective"),
    E("RECON_UNIT", 0x0e8e, 0x09b8, "opeu_mission_recon_unit"),
  )),
  CodeEnum("AngelAction", ("angel_actions",), "What the angel's remark does to the commander's unit (angel_actions; core/evangel).", (
    E("DEMORALIZE", 0x108c, 0x0db8, None),
    E("INSPIRE", 0x108c, 0x0e0e, None),
    E("JUDGE_SIDE", 0x108c, 0x0e4e, None),
  )),
  CodeEnum("ControlAction", ("control_actions",), "The command menu's entries (control_actions; core/control1).", (
    E("DELAY", 0x1389, 0x012c, "opeu_choose_turn_delay", "wait: the turn's delay"),
    E("ORDERS", 0x1c23, 0x0e86, "opeu_turn_action_1"),
    E("STOCKS", 0x1d1e, 0x01c2, "opeu_turn_action_2"),
    E("TRANSFER", 0x1d42, 0x01d2, "opeu_turn_action_3"),
    E("PERSONNEL", 0x1d82, 0x04c8, "opeu_turn_action_4"),
    E("STAFF", 0x1dd2, 0x20fc, "opeu_turn_action_5"),
    E("ORGANIZE", 0x20b3, 0x1f4e, "opeu_turn_action_6"),
    E("REQUEST", 0x22f9, 0x0970, "opeu_turn_action_7"),
    E("AIR_FORCE", 0x23b2, 0x10da, "opeu_turn_action_8"),
    E("SPECIAL_FORCES", 0x24d5, 0x0b26, "opeu_turn_action_9"),
    E("SETTINGS", 0x2597, 0x0d72, "opeu_turn_action_10"),
  )),
  CodeEnum("PersonnelAction", ("personnel_actions",), "The personnel menu's entries (personnel_actions; core/persmenu).", (
    E("REASSIGN", 0x1d82, 0x0084, "opeu_division_commander_menu"),
    E("APPOINT", 0x1d82, 0x010e, "opeu_commander_assign"),
    E("REST", 0x1d82, 0x0412, "opeu_commander_rest"),
  )),
  CodeEnum("AdviceAction", ("advice_actions",), "The advice menu's topics (advice_actions; core/advmenu).", (
    E("TACTICAL", 0x1dd2, 0x0a60, "opeu_advice_tactical"),
    E("REGIMENT_STATUS", 0x1dd2, 0x1140, "opeu_advice_regiment_status"),
    E("ENEMY_CONDITIONS", 0x1dd2, 0x1994, "opeu_advice_enemy_conditions"),
    E("TIDE_OF_WAR", 0x1dd2, 0x1e16, "opeu_advice_tide_of_war"),
    E("OPERATION_HINTS", 0x1dd2, 0x207a, "opeu_advice_operation_hints"),
  )),
  CodeEnum("StaffAction", ("staff_actions",), "The staff menu's officers, from 1 (staff_actions; core/advmenu).", (
    E("UNUSED", 0x0074, 0x6e61, None, "entry 0, never chosen: the bytes of a text"),
    E("OPERATIONS", 0x1dd2, 0x209a, "opeu_staff_advice_menu"),
    E("INTELLIGENCE", 0x1fe6, 0x0c90, "opeu_view_menu_loop"),
  )),
  CodeEnum("PeopleAction", ("people_actions",), "The generals view's sides (people_actions; core/vwpeople).", (
    E("OWN", 0x1fe6, 0x0656, "opeu_view_own_generals"),
    E("ENEMY", 0x1fe6, 0x06ae, "opeu_view_enemy_generals"),
  )),
  CodeEnum("CityAction", ("city_actions",), "The cities view's sides (city_actions; core/vwcity).", (
    E("OWN", 0x1fe6, 0x082e, "opeu_view_own_cities"),
    E("ENEMY", 0x1fe6, 0x085e, "opeu_view_enemy_cities"),
  )),
  CodeEnum("InfoAction", ("info_actions",), "The units view's sides (info_actions; core/vwarms).", (
    E("OWN", 0x1fe6, 0x09d6, "opeu_view_own_divisions"),
    E("ENEMY", 0x1fe6, 0x0a7a, "opeu_view_enemy_units"),
  )),
  CodeEnum("ArmsAction", ("arms_actions",), "The armaments view's sides (arms_actions; core/vwarms).", (
    E("OWN", 0x1fe6, 0x0ba6, "opeu_view_own_weapons"),
    E("ENEMY", 0x1fe6, 0x0be4, "opeu_view_enemy_weapons"),
  )),
  CodeEnum("ViewMainAction", ("view_main_actions",), "The view menu's entries (view_main_actions; core/vwarms).", (
    E("DIVISIONS", 0x1fe6, 0x04fa, "opeu_view_division_contact"),
    E("OFFICERS", 0x1fe6, 0x0724, "opeu_view_generals_menu"),
    E("CITIES", 0x1fe6, 0x0894, "opeu_view_cities_menu"),
    E("UNITS", 0x1fe6, 0x0b02, "opeu_view_info_menu"),
    E("ARMAMENTS", 0x1fe6, 0x0c28, "opeu_view_arms_menu"),
    E("VICTORY", 0x1fe6, 0x0c64, "opeu_view_victory"),
  )),
  CodeEnum("RequestHandler", ("request_handlers",), "The request menu's entries (request_handlers; core/rqmenu).", (
    E("SUPPLIES", 0x22f9, 0x0136, "opeu_request_supplies"),
    E("BATTALION", 0x22f9, 0x03b6, "opeu_request_arms"),
    E("REGIMENT", 0x22f9, 0x0780, "opeu_request_regiment"),
  )),
  CodeEnum("AirMission", ("air_missions",), "The air force menu's missions (air_missions; core/airforce).", (
    E("STRIKE", 0x23b2, 0x05b0, "opeu_air_bomb_command"),
    E("PARATROOPERS", 0x23b2, 0x0b28, "opeu_air_drop_command"),
    E("CARGO_DROP", 0x23b2, 0x0e20, "opeu_air_lift_command"),
  )),
  CodeEnum("SpecialForcesAction", ("special_missions",), "The special forces menu's missions (special_missions; core/specfrc).", (
    E("RECON", 0x24d5, 0x0840, "opeu_special_recon"),
    E("CUT_LINES", 0x24d5, 0x0414, "opeu_special_cut_comm"),
    E("SABOTAGE", 0x24d5, 0x0252, "opeu_special_sabotage"),
    E("RAID", 0x24d5, 0x0008, "opeu_special_raid"),
    E("ASSASSINATE", 0x24d5, 0x0668, "opeu_special_assassinate"),
  )),
  CodeEnum("SaveQuitAction", ("save_quit_actions",), "The save/quit menu's entries (save_quit_actions; core/settings).", (
    E("SAVE_GAME", 0x2597, 0x091e, None),
    E("NEW_GAME", 0x2597, 0x0cbc, None),
    E("LEAVE", 0x2597, 0x0ca6, None),
  )),
  CodeEnum("SettingsAction", ("settings_actions",), "The settings menu's entries (settings_actions; core/settings).", (
    E("BATTLES", 0x2597, 0x0084, None),
    E("DELAY", 0x2597, 0x00ec, None),
    E("MAP", 0x2597, 0x012a, None),
    E("SOUND", 0x2597, 0x0176, None),
    E("PICTURES", 0x2597, 0x01cc, None),
    E("SAVE_MENU", 0x2597, 0x0d32, None),
  )),
  CodeEnum("BattleHandler", ("battle_handlers",), "The battle's sound effects, by handler (battle_handlers; map/btldraw).", tuple(
    E(f"EFFECT_{index}", 0x0d92, 0x0008 * (index + 1), "opeu_control_begin" if index == 8 else f"opeu_sound_cue{index}") for index in range(10))),
  CodeEnum("OrderHandler", ("order_handlers",), "What an order does when its unit reaches it, by order kind (order_handlers; units/finalact).", (
    E("ARRIVE", 0x32ca, 0x0002, "opeu_order_arrive"),
    E("OCCUPY", 0x3131, 0x028a, "opeu_order_occupy_run"),
    E("ATTACK", 0x3131, 0x044c, "opeu_order_attack_run"),
    E("DEFEND", 0x3131, 0x0550, "opeu_order_defend_run"),
    E("LAY_MINES", 0x319e, 0x00a8, "opeu_engineer_lay_mines"),
    E("REMOVE_MINES", 0x319e, 0x0172, "opeu_engineer_remove_mines"),
    E("BUILD_BRIDGE", 0x319e, 0x02c0, "opeu_engineer_build_bridge"),
    E("DESTROY", 0x3131, 0x061a, "opeu_order_destroy_run"),
    E("SUPPLY_OBJECTIVE", 0x31f4, 0x0ca6, "opeu_order_supply_objective"),
    E("SUPPLY_UNIT", 0x31f4, 0x0d06, "opeu_order_supply_unit"),
    E("REPAIR_OBJECTIVE", 0x319e, 0x03c2, "opeu_engineer_repair_objective"),
    E("REPAIR_UNIT", 0x319e, 0x04a0, "opeu_engineer_repair_unit"),
    E("REACH_UNIT", 0x32ca, 0x004a, "opeu_order_reach_unit"),
    E("HOLD", 0x32ca, 0x00ac, "opeu_order_hold"),
  )),
)

PRESENTER = CodeEnum("Presenter", (), "What a daily show runs after its picture (opeu_daily_show; core/dialogs). Its values are the "
                     "presenters' original far addresses, segment << 16 | offset, like the code slots' enums (common/data-segment.hpp) "
                     "until M4 renumbers them.", (
  E("DAILY_EVENT", 0x0e36, 0x0000, "opeu_daily_present"),
  E("MISSION_FINISH", 0x0e8e, 0x0042, "opeu_mission_finish_present"),
  E("MISSION_DELAY", 0x0e8e, 0x004a, "opeu_mission_delay_present"),
  E("AIR_STRIKE", 0x0f43, 0x0406, "opeu_air_strike_present"),
  E("PARATROOPERS", 0x0f43, 0x0d72, "opeu_para_drop_present"),
  E("CARGO_DROP", 0x0f43, 0x1042, "opeu_cargo_drop_present"),
  E("VISIT", 0x108c, 0x02ac, "opeu_visit_present"),
  E("HURRICANE", 0x108c, 0x0396, "opeu_hurricane_present"),
  E("PARTISAN_LOSSES", 0x108c, 0x06ac, "opeu_loss_present_a"),
  E("NORTHERN_LOSSES", 0x108c, 0x0830, "opeu_loss_present_b"),
  E("BOMBING", 0x108c, 0x0aae, "opeu_bomb_present"),
  E("ANGEL", 0x108c, 0x0ef6, "opeu_angel_present"),
  E("SPY_GREETING", 0x108c, 0x10b8, "opeu_spy_present_a"),
  E("SPY_ENDING", 0x108c, 0x1132, "opeu_spy_present_b"),
  E("REPORT", 0x108c, 0x1302, "opeu_report_present"),
  E("DEATH", 0x11e5, 0x0004, "opeu_death_present"),
  E("REQUEST_ERROR", 0x1270, 0x02e4, "opeu_request_error_present"),
  E("REQUEST_START", 0x1270, 0x0430, "opeu_pending_start_present"),
  E("REQUEST_FALLBACK", 0x1270, 0x04f0, "opeu_pending_fallback_present"),
  E("ARRIVAL", 0x1270, 0x0816, "opeu_arrival_present"),
  E("BLAST_PAUSE", 0x17c4, 0x0866, "opeu_blast_pause"),
  E("TRANSFER", 0x1d42, 0x01ca, "opeu_transfer_present"),
  E("PERSONNEL_FINISH", 0x1d82, 0x0004, "opeu_personnel_finish"),
  E("ALLOCATION_FINISH", 0x20b3, 0x1f46, "opeu_alloc_finish"),
  E("REQUEST_FINISH", 0x22f9, 0x009a, "opeu_request_finish"),
  E("SPECIAL_FORCES", 0x24d5, 0x0000, "opeu_special_present"),
  E("ATTACK", 0x3131, 0x00f6, "opeu_order_attack_present"),
  E("OCCUPIED", 0x3131, 0x0282, "opeu_order_occupied_present"),
  E("BRIDGE_DESTROYED", 0x3131, 0x060c, "opeu_order_destroy_present"),
  E("MINES_LAID", 0x319e, 0x00a0, "opeu_mines_laid_finish"),
  E("MINES_REMOVED", 0x319e, 0x016a, "opeu_mines_removed_finish"),
  E("BRIDGE_BUILT", 0x319e, 0x02b8, "opeu_bridge_built_finish"),
  E("ARMS_REPAIRED", 0x319e, 0x03ba, "opeu_arms_repaired_finish"),
  E("TRANSFER_STEP", 0x31f4, 0x05a8, "opeu_transfer_step"),
  E("MINES_HIT", 0x32e6, 0x05a0, "opeu_move_mines_finish"),
  E("NOTICE_VIEW", 0x33ad, 0x05fe, "opeu_notice_view_finish"),
  E("END_OCCUPY", 0x3424, 0x0082, "opeu_end_occupy_present"),
))

ALL_ENUMS = (*SLOT_ENUMS, PRESENTER)

_PAIR = re.compile(r"\{\s*(0x[0-9a-fA-F]+|\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)\s*\}")
_TABLE_INIT = re.compile(r"data\.(\w+)\s*=\s*\{\s*\{(.*?)\}\s*\};", re.S)
_WINDOW_INIT = re.compile(r"(data\.(\w+)\s*=\s*\{)([^;{}]*\{[^{}]*\}\s*,\s*\{[^{}]*\}\s*)(\};)", re.S)
_NOON_ROW = re.compile(r"(\{\s*0x\w+\s*,\s*0x\w+\s*,\s*\{[^{}]*\}\s*,\s*)(\{[^{}]*\}),\s*")
_MISSION_BYTES = re.compile(r"data\.mission_handler_bytes\s*=\s*\{([^}]*)\};\s*\n(\s*)data\.month_climate(\s*)=\s*\{\s*(0x[0-9a-fA-F]+)", re.S)


@dataclass
class B4Result:
  files:     dict[str, str] = field(default_factory=dict)  # path relative to sources/opeu -> new text
  replaced:  dict[str, int] = field(default_factory=dict)  # path -> pairs replaced
  problems:  list[str] = field(default_factory=list)       # pairs or enumerators the model cannot account for
  handlers:  dict[str, int] = field(default_factory=dict)  # path -> port_routine_* handlers deleted

  def Counts(self) -> dict:
    return {"enums": len(ALL_ENUMS), "enumerators": sum(len(e.enumerators) for e in ALL_ENUMS),
            "slot_tables": sum(len(e.fields) for e in SLOT_ENUMS), "pairs": sum(self.replaced.values()), "handlers": sum(self.handlers.values()), "files": len(self.files),
            "problems": len(self.problems)}


def CheckAgainstLinks(entries: dict[int, tuple[str, str | None]]) -> list[str]:
  """Every enumerator naming a routine must be the routine the link table holds at its address (entry name or the
  routine its handler calls)."""
  problems = []
  for enum in ALL_ENUMS:
    for e in enum.enumerators:
      if e.routine is None:
        continue
      entry = entries.get(e.segment * 16 + e.offset)
      if entry is None or e.routine not in (entry[0], (entry[1] or "").removeprefix("port_routine_")):
        problems.append(f"{enum.name}::{e.name} {e.segment:04x}:{e.offset:04x}: the link table holds {entry}, not {e.routine}")
  return problems


def LinkEntries(text: str) -> dict[int, tuple[str, str | None]]:
  found = {}
  for match in re.finditer(r'\{\s*0x([0-9a-fA-F]+)u?\s*,\s*"([^"]+)"\s*,\s*(\w+)\s*\}', text):
    found[int(match.group(1), 16)] = (match.group(2), None if match.group(3) == "nullptr" else match.group(3))
  return found


def EnumBlock(enum: CodeEnum) -> str:
  lines = [f"// {line}" for line in textwrap.wrap(enum.comment, 117)] + [f"enum class {enum.name} : std::uint32_t {{"]
  for e in enum.enumerators:
    comment = f" // {e.comment}" if e.comment else ""
    lines.append(f"  {e.name} = 0x{e.value:08x},{comment}")
  lines.append("};")
  return "\n".join(lines) + "\n"


def PresenterHeader() -> str:
  return ("// The presenters a daily show (core/dialogs opeu_daily_show) runs after its picture.\n#pragma once\n\n#include <cstdint>\n\n"
          "namespace opeu::game::detail::presenter {\n\n" + EnumBlock(PRESENTER) + "\n} // namespace opeu::game::detail::presenter\n\n"
          "namespace opeu::game {\nusing detail::presenter::Presenter;\n} // namespace opeu::game\n")


def RewriteSegmentHeader(text: str) -> tuple[str, list[str]]:
  problems = []
  anchor = re.search(r"struct FarPointer \{.*?\n\};\n", text, re.S)
  if anchor is None:
    return text, ["data-segment.hpp: no FarPointer struct to put the enums after"]
  blocks = "".join("\n" + EnumBlock(enum) for enum in SLOT_ENUMS)
  text = text[:anchor.end()] + blocks + text[anchor.end():]
  for enum in SLOT_ENUMS:
    for name in enum.fields:
      pattern = re.compile(rf"(std::array<)FarPointer(,\s*\d+>\s+{name};)|\bFarPointer(\s+{name};)")
      text, count = pattern.subn(lambda m: f"{m.group(1)}{enum.name}{m.group(2)}" if m.group(1) else f"{enum.name}{m.group(3)}", text)
      if count != 1:
        problems.append(f"data-segment.hpp: {count} slots named {name}")
  exports = "".join(f"using detail::data_segment::{enum.name};\n" for enum in SLOT_ENUMS)
  text = text.replace("using detail::data_segment::FarPointer;\n", "using detail::data_segment::FarPointer;\n" + exports, 1)
  return text, problems


def RewriteInitialisers(path: str, text: str, windows: set[str]) -> tuple[str, int, list[str]]:
  """The code pairs of path's initialisers as enumerators (windows: the ListWindow slots, whose last two pairs are
  draw and select); returns the text, the pairs replaced and the problems."""
  by_field = {name: enum for enum in SLOT_ENUMS for name in enum.fields}
  problems: list[str] = []
  replaced = 0

  def Name(enum: CodeEnum, offset: str, segment: str, where: str) -> str:
    nonlocal replaced
    found = enum.ByPair().get((int(offset, 0), int(segment, 0)))
    if found is None:
      problems.append(f"{path}: {where}: {{{offset}, {segment}}} is no {enum.name}")
      return f"{{ {offset}, {segment} }}"
    replaced += 1
    return f"{enum.name}::{found.name}"

  def Table(match: re.Match) -> str:
    enum = by_field.get(match.group(1))
    if enum is None:
      return match.group(0)
    body = _PAIR.sub(lambda pair: Name(enum, pair.group(1), pair.group(2), match.group(1)), match.group(2))
    return match.group(0).replace(match.group(2), body)

  def Window(match: re.Match) -> str:
    pairs = list(_PAIR.finditer(match.group(3)))
    if len(pairs) != 2:
      return match.group(0)
    draw = Name(by_field["draw"], pairs[0].group(1), pairs[0].group(2), f"{match.group(2)}.draw")
    select = Name(by_field["select"], pairs[1].group(1), pairs[1].group(2), f"{match.group(2)}.select")
    body = match.group(3)[:pairs[0].start()] + draw + match.group(3)[pairs[0].end():pairs[1].start()] + select + match.group(3)[pairs[1].end():]
    return match.group(1) + body + match.group(4)

  def Noon(match: re.Match) -> str:
    pair = _PAIR.fullmatch(match.group(2))
    enum = by_field["action"]
    width = len(enum.name) + 2 + max(len(e.name) for e in enum.enumerators) + 2  # the column stays aligned (clang-format off)
    return match.group(1) + (Name(enum, pair.group(1), pair.group(2), "noon_events.action") + ",").ljust(width)

  def Missions(match: re.Match) -> str:
    raw = [int(byte, 0) for byte in re.findall(r"0x[0-9a-fA-F]+|\d+", match.group(1))] + [int(match.group(4), 0)]
    names = []
    for index in range(0, len(raw), 4):
      offset, segment = raw[index] | raw[index + 1] << 8, raw[index + 2] | raw[index + 3] << 8
      names.append(Name(by_field["mission_handlers"], hex(offset), hex(segment), "mission_handlers"))
    rows = "".join(f"{match.group(2)}  {name},\n" for name in names)
    return (f"data.mission_handlers = {{ {{\n{rows}{match.group(2)}}} }};\n"
            f"{match.group(2)}data.month_climate{match.group(3)}= {{ {match.group(4)}")

  text = _TABLE_INIT.sub(Table, text)
  if "data.noon_events" in text:
    start = text.index("data.noon_events")
    end = text.index("} };", start)
    text = text[:start] + _NOON_ROW.sub(Noon, text[start:end]) + text[end:]
  text = _WINDOW_INIT.sub(lambda m: Window(m) if m.group(2) in windows else m.group(0), text)
  text = _MISSION_BYTES.sub(Missions, text)
  return text, replaced, problems


_HANDLER = re.compile(r"^(?:auto|std::uint32_t|uint32_t) port_routine_\w+\([^)]*\)[^{;]*\{\n.*?^\}\n\n?", re.S | re.M)


def DeleteHandlers(text: str) -> tuple[str, int]:
  """Removes every `port_routine_*` handler definition (the link table's adapters); returns the text and how many."""
  text, count = _HANDLER.subn("", text)
  return re.sub(r"\n{3,}", "\n\n", text).rstrip("\n") + "\n", count


def B4Pass(tree: SourceTree) -> B4Result:
  result = B4Result()
  if LINK_TABLE in tree.files:
    result.problems += CheckAgainstLinks(LinkEntries(tree.files[LINK_TABLE].text))
  if SEGMENT_HEADER in tree.files:
    text, problems = RewriteSegmentHeader(tree.files[SEGMENT_HEADER].text)
    result.files[str(SEGMENT_HEADER)] = text
    result.problems += problems
  result.files[str(PRESENTER_HEADER)] = PresenterHeader()
  windows = set(re.findall(r"\bListWindow\s+(\w+);", tree.files[SEGMENT_HEADER].text)) if SEGMENT_HEADER in tree.files else set()
  for path, source in sorted(tree.files.items()):
    if not str(path).startswith("game/data/") or path.suffix != ".cpp":
      continue
    text, replaced, problems = RewriteInitialisers(str(path), source.text, windows)
    result.problems += problems
    if replaced:
      result.files[str(path)] = text
      result.replaced[str(path)] = replaced
  for path, source in sorted(tree.files.items()):
    if not str(path).startswith("game/") or str(path).startswith("game/link/"):
      continue
    text, count = DeleteHandlers(result.files.get(str(path), source.text))
    if count:
      result.files[str(path)] = text
      result.handlers[str(path)] = count
  return result


def WriteB4(root: Path, result: B4Result) -> None:
  for path, text in result.files.items():
    target = root / SOURCES / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)
