// MAIN's 16-bit data segment (DGROUP) as typed records: every link-map symbol a field at its original offset.
// Generated once from the link map, the data image and the game's accesses (docs/port.md "Data segment"); hand-owned.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "common/menu.hpp"

namespace opeu::game::detail::data_segment {

// Packed: the original records have odd strides. Arrays of bytes and of records are std::array; arrays of words are
// C arrays, which take the struct's packing (a std::array<std::uint16_t, N> would carry alignment 2 into it).
#pragma pack(push, 1)

// A far pointer as the original stores it: offset word, then segment word.
struct FarPointer {
  std::uint16_t offset;
  std::uint16_t segment;
};

// An order slot of a unit or a side: what it targets, which order, how far it has got.
struct Order {
  // a near pointer to the target record (objective or unit), or the map cell for orders that target a place
  union {
    std::uint16_t target;
    struct {
      std::uint8_t x;
      std::uint8_t y;
    } cell;
  };
  std::uint8_t kind;
  std::uint8_t progress;
};

// Order::kind of a unit's order; order_names[kind + 3] names it.
enum class UnitOrder : std::uint8_t {
  MOVE         = 0,    // move to the cell
  OCCUPY       = 1,    // take the objective at the cell
  ATTACK       = 2,    // attack the target unit
  DEFEND       = 3,    // hold the objective at the cell
  LAY_MINES    = 4,    // mine the cell
  CLEAR_MINES  = 5,    // clear the mines on the cell
  BUILD_BRIDGE = 6,    // bridge the river cell
  RESUPPLY     = 8,    // load supplies at the objective at the cell
  SUPPLY_UNIT  = 9,    // supply the target unit
  PURSUE       = 12,   // follow the target unit
  REST         = 13,   // fall back to the cell and rest
  NONE         = 0xff, // no order
};

// A filed request (core/requests): the same four bytes a side keeps for its objectives.
struct RequestSlot {
  std::uint16_t target; // near pointer to the requested record
  std::uint8_t  argument;
  std::uint8_t  kind;
};

// RequestSlot::kind and RequestConfig::kind: what a side asks headquarters for.
enum class RequestKind : std::uint8_t {
  SUPPLIES  = 0,    // supplies delivered to an objective (target)
  BATTALION = 1,    // a battalion of an equipment type (argument) at an objective (target)
  REGIMENT  = 2,    // a regiment of a kind (argument)
  NONE      = 0xff, // an empty slot
};

// A group's task: a map cell, the order kind and its progress.
struct Task {
  std::uint8_t x;
  std::uint8_t y;
  std::uint8_t kind;
  std::uint8_t progress;
};

// Task::kind: what the computer side's division is to do at the task's cell (core/groupai).
enum class TaskKind : std::uint8_t {
  CAPTURE = 0,    // take the enemy objective there
  HOLD    = 1,    // hold the own objective there
  MOVE    = 2,    // move into the quarter of the map around the cell
  NONE    = 0xff, // an unused task slot
};

// Equipment held by an objective or carried by an element: type index, quantity, extra word.
struct Item {
  std::uint8_t  type; // index into element_types
  std::uint16_t quantity;
  std::uint16_t extra;
};

// A side's air mission (core/airstrk, core/airbomb): target cell, planes, delay and turns.
struct AirStrike {
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint16_t planes;
  std::uint8_t  hours; // the delay before the strike (S_DELAY_P, ORDER_HOURS)
  std::uint8_t  turns;
};

// A side's airborne drop (core/airdrop, core/paradrop): target cell, the paratroops and the transport planes that
// carry them (30 troops a plane; 0 planes: no drop pending) and two hourly countdowns.
struct AirDrop {
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint16_t paratroops;
  std::uint16_t transports;
  std::uint8_t  hours; // hours until the planes fly; set again to 1 when the air command holds the drop an hour
  std::uint8_t  due;   // ordered as hours + 6; counts down with it and is what the air command weighs
};

// A side's supply airlift (core/aiops2, core/airlift): the cell supplied, the ammunition, fuel and food carried, the
// transport planes that carry them (0: no airlift pending), the hours until they fly and the hours the order lasts.
struct AirLift {
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint16_t supplies[3];
  std::uint16_t planes;
  std::uint8_t  hours;
  std::uint8_t  turns;
};

// The planes a bombing mission type sends: base + a random number below range.
struct BombingFlight {
  std::uint8_t base;
  std::uint8_t range;
};

// One of the two sides (DS:021a, 2 x 65 bytes).
struct Side {
  std::uint16_t stock
      [3]; // ammunition, fuel, food in the side's pool (SIDE_AMMO/FUEL/FOOD)
  std::uint16_t income[3];
  std::uint16_t pool_a; // the side's bombers ready to fly (strikes and bombing missions)
  std::uint16_t planes; // SIDE_PLANES
  std::uint16_t pending_a; // bombers back from a strike; added to pool_a at midnight
  std::uint16_t pending_planes; // added to planes at midnight (capped at 999)
  std::uint16_t paratroops; // the side's paratroopers not committed to a drop
  std::uint8_t
      strength; // STRENGTH (core/cmdxfer, core/paraunit), MORALE (core/airstrk)
  // the objective orders (settings, rebase) and the filed requests (core/requests) share these bytes
  union {
    std::array<Order, 3>       objectives;
    std::array<RequestSlot, 3> requests;
  };
  AirStrike strike; // AiSide plan; SIDE_W25 is strike.planes
  AirDrop   drop;   // the pending airborne drop
  AirLift                     lift;   // the pending supply airlift
  Order                       order;
};

// A general (DS:029c, 2 sides x 20); the person templates (DS:3160) have the same layout.
struct Person {
  std::array<char, 18> name;
  std::uint8_t         status;     // STATUS, PERSON_MASK, PERSON_POST
  std::uint8_t         leadership; // PRINT_STATS: Leader
  std::uint8_t         charm;      // PRINT_STATS: Charm
  std::uint8_t         bravery;    // PRINT_STATS: Brvy
  std::uint8_t         health;     // PRINT_STATS: Health
  std::uint8_t byte_17;    // PERSON_B23 (core/advregt compares it with health)
  std::uint8_t infantry;   // PRINT_STATS: Inf
  std::uint8_t artillery;  // PRINT_STATS: Arty
  std::uint8_t armor;      // PRINT_STATS: Tanks
  std::uint8_t experience; // PRINT_STATS: Exp; PERSON_1B
  std::uint8_t assignment; // ASSIGNMENT, POST, STATUS_BITS
  std::uint8_t portrait;   // PERSON_IMAGE
  // ROLE (low three bits) and FLAGS (0x40: health shown in red) read the same byte
  union {
    std::uint8_t role;
    std::uint8_t flags;
  };
};

// A group (division) of a side (DS:0774, 2 sides x 4).
struct Group {
  std::array<Task, 4> tasks;
  std::uint8_t        flags; // GROUP_FLAGS
};

// A battalion (DS:07fc, 2 sides x 128), linked from its unit through next.
struct Element {
  std::uint16_t
      next; // near pointer to the next Element of the unit, 0 at the end
  std::uint8_t  type;     // index into element_types
  std::uint16_t quantity; // ELEMENT_AMOUNT, QUANTITY
  std::uint16_t damage;   // ELEMENT_DAMAGE; Item::extra in the load views
  // battle position: POS reads the word, POS_X/POS_Y the bytes (x, y)
  union {
    std::uint16_t               position;
    std::array<std::uint8_t, 2> cell;
  };
  std::uint8_t morale;     // ELEMENT_MORALE, B9
  std::uint8_t training;   // ELEMENT_TRAINING, B10; btlcalc reads it as FATIGUE
  std::uint8_t experience; // EXPERIENCE, B11; tblelem prints it as fatigue
  std::uint8_t flags;      // ELEMENT_FLAGS
};

// A regiment (DS:14fc, 2 sides x 4 groups x 8).
struct Unit {
  std::uint16_t first; // near pointer to the unit's first Element, 0 when empty
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint8_t  target_x; // UNIT_TX
  std::uint8_t  target_y; // UNIT_TY
  // ammunition, fuel, food (UNIT_W6/W8/W10, UNIT_AMMO/FUEL/FOOD); units/xferunit reads the bytes
  union {
    std::uint16_t               resources[3];
    std::array<std::uint8_t, 6> resource_bytes;
  };
  std::array<Order, 2>
      orders; // orders[0].kind is UNIT_ORDER(S), orders[1].kind the saved order
  std::uint8_t moved; // UNIT_MOVED, UNIT_MARK, UNIT_POINTS
  std::uint8_t flags; // UNIT_FLAGS
};

// A city or other objective (DS:1a7c, 21 records).
struct Objective {
  // the name; a nonzero first byte marks the objective in use
  union {
    std::array<char, 21> name;
    std::uint8_t         active;
  };
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint16_t resources
      [3]; // ammunition, fuel, food (OBJECTIVE_AMMO/FUEL/FOOD, OBJECTIVE_W23/W25)
  std::uint16_t       reserves[3];    // core/stockord
  std::uint8_t        strength;       // GARRISON, OBJ_STRENGTH, OBJECTIVE_END
  std::uint8_t        strength_limit; // LIMIT, OBJECTIVE_END_MAX, OBJ_POINTS
  std::array<Item, 6> items;
  std::uint8_t        flags; // OBJECTIVE_FLAGS
};

// An equipment type (DS:3332, 158 records): name and ratings.
struct ElementType {
  std::uint16_t name;        // near pointer to the type's name string
  std::uint8_t  sides;       // TYPE_MASK, TYPE_FLAG: which sides field it
  std::uint8_t  date;        // TYPE_DATE
  std::uint8_t  category;    // TYPE_CATEGORY, CAPACITY, COST
  std::uint8_t  soft_rating; // SOFT_RATING, TYPE_STAT5
  std::uint8_t  hard_rating; // HARD_RATING, TYPE_STAT6
  std::uint8_t  range;       // RANGE, TYPE_STAT7
  std::uint8_t  speed;       // SPEED, TYPE_STAT8
  std::uint8_t  usage;       // TYPE_USAGE, TYPE_STAT9
  std::uint8_t  weight;      // TYPE_STAT10; weight_b in units/resagg
  std::uint8_t  mode;        // MODE, TYPE_STAT11
  std::uint8_t  flags;       // TYPE_FLAGS
};

// The dated-event view of the same 13-byte records (core/daylist).
struct DailyRecord {
  std::uint16_t               name;
  std::uint8_t                flags;
  std::uint8_t                date;
  std::uint8_t                kind;
  std::array<std::uint8_t, 7> unknown_05;
  std::uint8_t                state;
};

// A request configuration (DS:0142, 2 records; core/requests, core/reqpend).
struct RequestConfig {
  std::uint16_t target; // near pointer
  std::uint8_t  argument;
  std::uint8_t  kind; // CONFIG_MODE
  std::uint8_t  delay;
  std::uint16_t enabled[3]; // SET_SWITCH
  std::uint8_t  rate0;
  std::uint8_t  rate1;
};

// A saved-game directory entry (DS:d006, 20 slots; core/settings).
struct SaveSlot {
  std::uint8_t  scenario;
  std::uint8_t  delivery;
  std::uint8_t  year;
  std::uint8_t  month;
  std::uint8_t  day;
  std::uint8_t  hour;
  std::uint16_t control[2];
  std::uint16_t checksum;
};

// An engaged element as the battle began (core/notice fills, core/notend reports the losses): its unit (0 for the
// garrison), the element, and its equipment and quantity then.
struct BattleLoss {
  std::uint16_t unit;
  std::uint16_t element; // 0 for an unused entry
  Item          item;
};

// A dated noon event (core/noonevt): on the day it happens at noon the news is shown, the action runs and a
// leader may speak.
struct NoonEvent {
  std::uint8_t date;   // the year in the high nibble (from 1940), the month in the low nibble; 0xff ends the table
  std::uint8_t day;
  FarPointer   news;   // the news text
  FarPointer   action; // a far routine, or null
  std::uint8_t leader; // the audience in the low two bits, the person template above them; 0xff for no speech
  std::uint8_t line;   // index into noon_speeches
};

// A list window (io/listui; list_window points to the active one): its frame in 8-pixel columns and pixels, the
// column widths, the row count and the far row-draw and select callbacks.
struct ListWindow {
  std::uint16_t x;      // left, in 8-pixel columns
  std::uint16_t y;      // top, in pixels
  std::uint16_t right;  // right column
  std::uint16_t bottom; // bottom pixel
  std::uint16_t widths; // near pointer to the width bytes: the row width, then one per column, 0-terminated
  std::uint8_t  rows;
  FarPointer    draw;
  FarPointer    select;
};

// A node of the portrait or face cache (core/uiprim): the index of the next node, 0xff at the end of the list, and
// the key of the image held in the node's slot, 0xff for none.
struct CacheNode {
  std::uint8_t next;
  std::uint8_t key;
};

// The messages of an order dialog (the Orders command and the map cursor's unit action keep one copy each).
struct OrderPrompts {
  std::array<char, 18> your_orders;      // "Your orders, sir?"
  std::array<char, 31> no_repair_unit;   // "Repair unit does not exist."
  std::array<char, 43> select_target;    // "Please select the target coordinates, sir."
  std::array<char, 22> not_enemy_city;   // "Not an enemy city!"
  std::array<char, 10> fix_which;        // "Fix which"
  std::array<char, 13> supply_which;     // "Supply which"
  std::array<char, 35> cannot_bridge;    // "We cannot build a bridge there."
  std::array<char, 29> no_bridge;        // "There is no bridge there."
  std::array<char, 19> no_enemy;         // "No enemy there."
  std::array<char, 37> nothing_to_chase; // "No regiment to pursue there, sir."
  std::array<char, 24> cannot_order;     // "You cannot %s there."
  std::array<char, 31> understood;       // "Understood. I'll have them %s."
};

// A point on the screen in text columns and pixels.
struct ScreenPoint {
  std::uint16_t x;
  std::uint16_t y;
};

// Bits of Unit::flags.
enum class UnitFlag : std::uint8_t {
  SELECTED = 0x01, // the unit selected in its division (units/select)
  COMMUNICATIONS = 0x02, // in communication with its command; lost and restored by events (core/evunits)
  READY = 0x04, // "ready" (core/advregt, core/advtide): fielded and selectable
  REQUESTED = 0x08, // the free slot the requested regiment is built in (core/requnit)
  AIRBORNE = 0x10, // set aside for the side's pending airborne drop (core/airdrop, core/paradrop)
  SPOTTED = 0x20, // reconnoitred by the enemy: its orders show and missions may target it (core/msnrecon)
  ARRIVING = 0x40, // activated by core/arrivals
};

// Bits of Group::flags.
enum class GroupFlag : std::uint8_t {
  RESERVE  = 0x01, // a reserve division (core/divstr)
  SELECTED = 0x02, // the division selected on its side (units/select)
  READY   = 0x04, // an active division (core/advenemy, core/aiops5)
  AI_MARK =
      0x08, // set on every ready group by the computer side (core/aiops5 1ac2:15c0)
  RETARGET = 0x10, // the computer side's objective ranking changed: the division plans its tasks again (core/objrank, core/groupai)
};

// Bits of Element::flags.
enum class ElementFlag : std::uint8_t {
  SELECTED = 0x01, // the element selected in its unit (units/select)
  SPOTTED = 0x02, // reconnoitred by the enemy (core/msnrecon); counted by units/vwlist
  READY = 0x04, // linked into a unit (units/elemlink)
  REQUESTED = 0x08, // a battalion of the requested regiment (core/requnit)
  AIRBORNE = 0x10, // set aside for the side's pending airborne drop (core/airdrop, core/paradrop)
  BIT_5 = 0x20, // cleared when the element's unit arrives (core/arrivals)
  SIDE_1 = 0x80, // in a battle, the element fights for side 1 (map/btlend sets it, map/btllist reads it)
};

// Objective::flags: the low nibble is the objective's level (core/objrank), 0x10 the holding side, 0x40 a marker bit.
enum class ObjectiveFlag : std::uint8_t {
  LEVEL_MASK = 0x0f,
  SIDE_1     = 0x10, // held by side 1 (map/query); clear: held by side 0
  COMMUNICATIONS = 0x20, // in communication with its command (core/evcomms restores it)
  MARKED     = 0x40, // reconnoitred by the enemy (core/msnrecon)
};

// Person::role: the low three bits index role_names; 0x40 shows the health in red, 0x80 marks a deputy (core/deputy).
enum class PersonRole : std::uint8_t {
  ROLE_MASK   = 0x07,
  STAFF_TRAIT = 0x08, // rolled for each staff candidate, less often at a higher delivery level (core/staffgen)
  POOR_HEALTH = 0x40,
  DEPUTY      = 0x80,
};

// Person::role & ROLE_MASK: the general's staff post (role_names[role - 1] names it); 0 for none.
enum class StaffRole : std::uint8_t {
  NONE                     = 0,
  ARMY_COMMANDER           = 1, // the army's commander, who must not fall
  OPERATIONS_CHIEF         = 2,
  INTELLIGENCE_CHIEF       = 3,
  SUPREME_COMMANDER        = 4,
  CHIEF_OF_STAFF           = 5,
  AIR_FORCE_COMMANDER      = 6,
  SPECIAL_FORCES_COMMANDER = 7,
};

// Person::assignment: the low two bits are the division, 0x04 marks a general assigned to it; the top five bits
// are the general's counter (core/counter).
enum class Assignment : std::uint8_t {
  DIVISION_MASK = 0x03,
  DIVISION      = 0x04,
};

// Person::status of an unused record (otherwise the general's nation bits, matched against side_masks).
inline constexpr std::uint8_t PERSON_VACANT{ 0xff };

// DataSegment::side_control (and side_status) of a side a player commands; 0 when the computer commands it.
inline constexpr std::uint16_t SIDE_PLAYED{ 1 };

// DataSegment::side_stance: whether the side's scenario has it attack or defend.
enum class Stance : std::uint8_t {
  DEFENSIVE = 0,
  OFFENSIVE = 1,
};

// Bits of DataSegment::schedule_flags[0]: the rounds due before the next regiment's turn (core/app sets them, core/dispatch
// runs and clears them).
enum class ScheduleFlag : std::uint8_t {
  MONTHLY  = 0x01, // set on the first day of a month
  HALF_DAY = 0x02, // at hours 0 and 12
  HOURLY   = 0x04, // every hour
};

// Bits of DataSegment::side_flags, a side's state for the day.
enum class SideFlag : std::uint8_t {
  SPY_SENT         = 0x01, // core/evspy: the spy went out (at 15:00)
  SPY_DONE         = 0x02, // core/evspy: the spy's report is in
  VISITED          = 0x04, // core/evvisit: the day's visit took place
  AIR_CARGO_READY  = 0x08, // set at midnight, cleared when the side's cargo flight lands (core/aircargo)
  AIR_DROP_READY   = 0x10, // set at midnight, cleared when the side's airborne drop lands (core/aircargo)
  RESERVES_PENDING = 0x20, // a reserve division waits to arrive (core/noonevt, core/arrivals)
  TURN_STARTED     = 0x80, // the side's turn state is initialised (core/dispatch)
};

// Bits of ElementType::flags as units/value weighs them: a heavy type is worth 100 points, a light tank type counts
// like ten infantry.
enum class TypeFlag : std::uint8_t {
  AIRBORNE = 0x01, // a paratroop type (core/airdrop)
  BIT_2 = 0x04, // a tank type not LIGHT drawn with the third tank icon in battle (map/btlicon)
  HEAVY = 0x08,
  LIGHT = 0x10,
};

// Bits of a map_cells byte: the terrain (low nibble), which side's unit stands on the cell (map/occupy) and which
// sides marked it (map/cellflag).
enum class MapCell : std::uint8_t {
  TERRAIN_MASK  = 0x0f,
  SIDE_0        = 0x10, // a unit of side 0
  SIDE_1        = 0x20, // a unit of side 1
  OCCUPANT_MASK = 0x30,
  MARK_0        = 0x40, // marked by side 0
  MARK_1        = 0x80, // marked by side 1
  MARK_MASK     = 0xc0,
};

// The terrain of a map cell (the low nibble of its map_cells byte), indexing terrain_names.
enum class Terrain : std::uint8_t {
  IMPASSABLE = 0,
  CITY       = 1, // an objective stands there
  ROAD       = 2,
  PLAINS     = 3,
  DESERT     = 4,
  RIVER      = 5, // a destroyed bridge becomes river
  SWAMP      = 6,
  WASTELAND  = 7,
  FOREST     = 8,
  HILL       = 9,
  BRIDGE     = 10,
};

// DataSegment::battle_view: which battles the player watches (the Settings command and the scenario setup set it).
enum class BattleView : std::uint16_t {
  NONE   = 0,    // no battles
  ALL    = 1,    // every battle
  SELECT = 0xff, // asked before each battle
};

// ElementType::category (core/divstr).
enum class Category : std::uint8_t {
  INFANTRY     = 0, // counts x1
  INFANTRY_X10 = 1,
  INFANTRY_X5  = 2,
  TANKS        = 3,
  ARTILLERY    = 4,
};

// The value of a flag or code as the byte the record stores.
template <typename E>
[[nodiscard]] constexpr auto Bits(E value) noexcept -> std::uint8_t {
  return static_cast<std::uint8_t>(value);
}

// The whole segment, DS:0000..ffff. Gaps are unknown_XXXX; stack is the DS stack from DS:ebe0.
struct DataSegment {
  std::array<std::uint8_t, 66> unknown_0000; // DS:0000
  union {
    std::array<std::uint8_t, 256> scenario_header; // DS:0042..0142
    struct {
      std::uint8_t scenario_first; // DS:0042
      std::uint8_t scenario;       // DS:0043
      std::uint8_t year;           // DS:0044
      std::uint8_t month;          // DS:0045
      std::uint8_t day;            // DS:0046
      std::uint8_t hour;           // DS:0047
      std::uint8_t weather;        // DS:0048
      std::uint8_t map_scale;      // DS:0049
      union {                      // DS:004a
        std::uint8_t message_delay;
        std::uint8_t message_speed;
      };
      std::uint8_t delivery_level; // DS:004b
      union {                      // DS:004c
        std::array<std::uint8_t, 2> map_mode;
        std::array<std::uint8_t, 2> arrival_mode;
      };
      union { // DS:004e
        std::uint16_t event_pictures;
        std::uint16_t message_pictures;
      };
      union { // DS:0050
        std::uint16_t               sound_enabled;
        std::array<std::uint8_t, 2> sound_enabled_bytes;
      };
      std::uint16_t               battle_view; // DS:0052
      std::array<std::uint8_t, 2> unknown_0054;
      std::array<std::uint8_t, 2> side_flags; // DS:0056
      union {
        std::array<std::uint8_t, 2> side_delays; // DS:0058..005a
        struct {
          std::uint8_t counter0; // DS:0058
          std::uint8_t counter1; // DS:0059
        };
      };
      std::uint16_t side_status[2]; // DS:005a
      union {                       // DS:005e
        std::uint16_t               side_control[2];
        std::array<std::uint8_t, 4> side_control_bytes;
      };
      std::uint8_t                current_side; // DS:0062
      std::array<std::uint8_t, 1> unknown_0063;
      std::uint16_t               current_side_ptr; // DS:0064
      std::uint8_t                current_group;    // DS:0066
      std::array<std::uint8_t, 1> unknown_0067;
      std::uint16_t               current_group_ptr; // DS:0068
      std::uint8_t                current_entry;     // DS:006a
      std::array<std::uint8_t, 1> unknown_006b;
      union { // DS:006c
        std::uint16_t               current_unit_ptr;
        std::array<std::uint8_t, 2> current_unit_ptr_bytes;
      };
      std::uint16_t context_record1; // DS:006e
      union {                        // DS:0070
        std::uint16_t               context_record2;
        std::array<std::uint8_t, 2> context_record2_bytes;
      };
      union { // DS:0072
        std::uint16_t               context_record3;
        std::array<std::uint8_t, 2> context_record3_bytes;
      };
      union { // DS:0074
        std::uint16_t               context_group_record;
        std::array<std::uint8_t, 2> context_group_record_bytes;
      };
      union { // DS:0076
        std::array<std::uint8_t, 56> terrain_ratings;
        std::array<std::uint8_t, 56> move_costs;
      };
      std::array<std::uint8_t, 144> weather_odds;     // DS:00ae
      std::array<std::uint8_t, 2>   side_masks; // DS:013e: the nation bits of each side (Person::status, ElementType::sides)
      std::array<std::uint8_t, 2>   side_stance;      // DS:0140
    };
  };
  union { // DS:0142
    std::array<RequestConfig, 2> special;
    std::array<RequestConfig, 2> request_configs;
  };
  std::array<std::uint8_t, 158> request_weights1;       // DS:015c
  std::uint16_t                 request_directions[16]; // DS:01fa
  std::array<Side, 2>           sides;                  // DS:021a
  std::array<Person, 40>        people;                 // DS:029c
  std::array<Group, 8>          groups;                 // DS:0774
  std::array<Element, 256>      elements;               // DS:07fc
  std::array<Unit, 64>          units;                  // DS:14fc
  std::array<Objective, 21>     objectives;             // DS:1a7c
  std::uint8_t                  scenario_last;          // DS:2010
  union {                                               // DS:2011
    std::uint8_t scenario_after;
    std::uint8_t turn_cursor;
  };
  std::array<std::uint8_t, 64> turn_order; // DS:2012
  union {                                  // DS:2052
    std::uint16_t side_entry_values[42];
    std::uint16_t objective_links[42];
  };
  std::uint16_t                  unit_links[8];      // DS:20a6
  std::array<std::uint8_t, 1600> map_cells;          // DS:20b6
  std::int16_t                   influence_map[100]; // DS:26f6
  union {
    std::array<std::uint8_t, 1314> unit_type_names; // DS:27be..2ce0
    struct {
      std::uint8_t         save_last; // DS:27be
      std::uint8_t         save_end;  // DS:27bf
      std::array<char, 11> text_27c0; // DS:27c0
      std::array<char, 12> text_27cb; // DS:27cb
      std::array<char, 14> text_27d7; // DS:27d7
      std::array<char, 14> text_27e5; // DS:27e5
      std::array<char, 10> text_27f3; // DS:27f3
      std::array<char, 14> text_27fd; // DS:27fd
      std::array<char, 12> text_280b; // DS:280b
      std::array<char, 13> text_2817; // DS:2817
      std::array<char, 9>  text_2824; // DS:2824
      std::array<char, 10> text_282d; // DS:282d
      std::array<char, 11> text_2837; // DS:2837
      std::array<char, 11> text_2842; // DS:2842
      std::array<char, 12> text_284d; // DS:284d
      std::array<char, 12> text_2859; // DS:2859
      std::array<char, 12> text_2865; // DS:2865
      std::array<char, 12> text_2871; // DS:2871
      std::array<char, 12> text_287d; // DS:287d
      std::array<char, 11> text_2889; // DS:2889
      std::array<char, 12> text_2894; // DS:2894
      std::array<char, 12> text_28a0; // DS:28a0
      std::array<char, 11> text_28ac; // DS:28ac
      std::array<char, 11> text_28b7; // DS:28b7
      std::array<char, 10> text_28c2; // DS:28c2
      std::array<char, 10> text_28cc; // DS:28cc
      std::array<char, 8>  text_28d6; // DS:28d6
      std::array<char, 9>  text_28de; // DS:28de
      std::array<char, 10> text_28e7; // DS:28e7
      std::array<char, 10> text_28f1; // DS:28f1
      std::array<char, 13> text_28fb; // DS:28fb
      std::array<char, 14> text_2908; // DS:2908
      std::array<char, 9>  text_2916; // DS:2916
      std::array<char, 11> text_291f; // DS:291f
      std::array<char, 12> text_292a; // DS:292a
      std::array<char, 8>  text_2936; // DS:2936
      std::array<char, 10> text_293e; // DS:293e
      std::array<char, 10> text_2948; // DS:2948
      std::array<char, 9>  text_2952; // DS:2952
      std::array<char, 7>  text_295b; // DS:295b
      std::array<char, 9>  text_2962; // DS:2962
      std::array<char, 9>  text_296b; // DS:296b
      std::array<char, 6>  text_2974; // DS:2974
      std::array<char, 7>  text_297a; // DS:297a
      std::array<char, 9>  text_2981; // DS:2981
      std::array<char, 8>  text_298a; // DS:298a
      std::array<char, 8>  text_2992; // DS:2992
      std::array<char, 8>  text_299a; // DS:299a
      std::array<char, 8>  text_29a2; // DS:29a2
      std::array<char, 11> text_29aa; // DS:29aa
      std::array<char, 11> text_29b5; // DS:29b5
      std::array<char, 11> text_29c0; // DS:29c0
      std::array<char, 11> text_29cb; // DS:29cb
      std::array<char, 12> text_29d6; // DS:29d6
      std::array<char, 11> text_29e2; // DS:29e2
      std::array<char, 12> text_29ed; // DS:29ed
      std::array<char, 10> text_29f9; // DS:29f9
      std::array<char, 9>  text_2a03; // DS:2a03
      std::array<char, 3>  text_2a0c; // DS:2a0c
      std::array<char, 7>  text_2a0f; // DS:2a0f
      std::array<char, 16> text_2a16; // DS:2a16
      std::array<char, 4>  text_2a26; // DS:2a26
      std::array<char, 6>  text_2a2a; // DS:2a2a
      std::array<char, 6>  text_2a30; // DS:2a30
      std::array<char, 11> text_2a36; // DS:2a36
      std::array<char, 10> text_2a41; // DS:2a41
      std::array<char, 14> text_2a4b; // DS:2a4b
      std::array<char, 8>  text_2a59; // DS:2a59
      std::array<char, 4>  text_2a61; // DS:2a61
      std::array<char, 9>  text_2a65; // DS:2a65
      std::array<char, 16> text_2a6e; // DS:2a6e
      std::array<char, 16> text_2a7e; // DS:2a7e
      std::array<char, 10> text_2a8e; // DS:2a8e
      std::array<char, 11> text_2a98; // DS:2a98
      std::array<char, 11> text_2aa3; // DS:2aa3
      std::array<char, 11> text_2aae; // DS:2aae
      std::array<char, 12> text_2ab9; // DS:2ab9
      std::array<char, 13> text_2ac5; // DS:2ac5
      std::array<char, 12> text_2ad2; // DS:2ad2
      std::array<char, 13> text_2ade; // DS:2ade
      std::array<char, 9>  text_2aeb; // DS:2aeb
      std::array<char, 10> text_2af4; // DS:2af4
      std::array<char, 11> text_2afe; // DS:2afe
      std::array<char, 10> text_2b09; // DS:2b09
      std::array<char, 11> text_2b13; // DS:2b13
      std::array<char, 12> text_2b1e; // DS:2b1e
      std::array<char, 14> text_2b2a; // DS:2b2a
      std::array<char, 13> text_2b38; // DS:2b38
      std::array<char, 4>  text_2b45; // DS:2b45
      std::array<char, 9>  text_2b49; // DS:2b49
      std::array<char, 8>  text_2b52; // DS:2b52
      std::array<char, 9>  text_2b5a; // DS:2b5a
      std::array<char, 10> text_2b63; // DS:2b63
      std::array<char, 10> text_2b6d; // DS:2b6d
      std::array<char, 10> text_2b77; // DS:2b77
      std::array<char, 7>  text_2b81; // DS:2b81
      std::array<char, 6>  text_2b88; // DS:2b88
      std::array<char, 11> text_2b8e; // DS:2b8e
      std::array<char, 14> text_2b99; // DS:2b99
      std::array<char, 11> text_2ba7; // DS:2ba7
      std::array<char, 14> text_2bb2; // DS:2bb2
      std::array<char, 15> text_2bc0; // DS:2bc0
      std::array<char, 13> text_2bcf; // DS:2bcf
      std::array<char, 9>  text_2bdc; // DS:2bdc
      std::array<char, 14> text_2be5; // DS:2be5
      std::array<char, 4>  text_2bf3; // DS:2bf3
      std::array<char, 10> text_2bf7; // DS:2bf7
      std::array<char, 7>  text_2c01; // DS:2c01
      std::array<char, 10> text_2c08; // DS:2c08
      std::array<char, 11> text_2c12; // DS:2c12
      std::array<char, 9>  text_2c1d; // DS:2c1d
      std::array<char, 11> text_2c26; // DS:2c26
      std::array<char, 10> text_2c31; // DS:2c31
      std::array<char, 14> text_2c3b; // DS:2c3b
      std::array<char, 4>  text_2c49; // DS:2c49
      std::array<char, 4>  text_2c4d; // DS:2c4d
      std::array<char, 5>  text_2c51; // DS:2c51
      std::array<char, 8>  text_2c56; // DS:2c56
      std::array<char, 7>  text_2c5e; // DS:2c5e
      std::array<char, 4>  text_2c65; // DS:2c65
      std::array<char, 4>  text_2c69; // DS:2c69
      std::array<char, 5>  text_2c6d; // DS:2c6d
      std::array<char, 5>  text_2c72; // DS:2c72
      std::array<char, 6>  text_2c77; // DS:2c77
      std::array<char, 6>  text_2c7d; // DS:2c7d
      std::array<char, 6>  text_2c83; // DS:2c83
      std::array<char, 7>  text_2c89; // DS:2c89
      std::array<char, 7>  text_2c90; // DS:2c90
      std::array<char, 8>  text_2c97; // DS:2c97
      std::array<char, 8>  text_2c9f; // DS:2c9f
      std::array<char, 8>  text_2ca7; // DS:2ca7
      std::array<char, 8>  text_2caf; // DS:2caf
      std::array<char, 9>  text_2cb7; // DS:2cb7
      std::array<char, 11> text_2cc0; // DS:2cc0
      std::array<char, 11> text_2ccb; // DS:2ccb
      std::array<char, 10> text_2cd6; // DS:2cd6
    };
  };
  union {
    std::array<std::uint8_t, 428> general_names; // DS:2ce0..2e8c
    struct {
      std::array<char, 6>  text_2ce0; // DS:2ce0
      std::array<char, 7>  text_2ce6; // DS:2ce6
      std::array<char, 5>  text_2ced; // DS:2ced
      std::array<char, 5>  text_2cf2; // DS:2cf2
      std::array<char, 4>  text_2cf7; // DS:2cf7
      std::array<char, 7>  text_2cfb; // DS:2cfb
      std::array<char, 5>  text_2d02; // DS:2d02
      std::array<char, 7>  text_2d07; // DS:2d07
      std::array<char, 4>  text_2d0e; // DS:2d0e
      std::array<char, 4>  text_2d12; // DS:2d12
      std::array<char, 4>  text_2d16; // DS:2d16
      std::array<char, 4>  text_2d1a; // DS:2d1a
      std::array<char, 4>  text_2d1e; // DS:2d1e
      std::array<char, 4>  text_2d22; // DS:2d22
      std::array<char, 11> text_2d26; // DS:2d26
      std::array<char, 10> text_2d31; // DS:2d31
      std::array<char, 11> text_2d3b; // DS:2d3b
      std::array<char, 14> text_2d46; // DS:2d46
      std::array<char, 15> text_2d54; // DS:2d54
      std::array<char, 16> text_2d63; // DS:2d63
      std::array<char, 21> text_2d73; // DS:2d73
      std::array<char, 7>  text_2d88; // DS:2d88
      std::array<char, 7>  text_2d8f; // DS:2d8f
      std::array<char, 7>  text_2d96; // DS:2d96
      std::array<char, 7>  text_2d9d; // DS:2d9d
      std::array<char, 8>  text_2da4; // DS:2da4
      std::array<char, 8>  text_2dac; // DS:2dac
      std::array<char, 7>  text_2db4; // DS:2db4
      std::array<char, 7>  text_2dbb; // DS:2dbb
      std::array<char, 12> text_2dc2; // DS:2dc2
      std::array<char, 5>  text_2dce; // DS:2dce
      std::array<char, 5>  text_2dd3; // DS:2dd3
      std::array<char, 7>  text_2dd8; // DS:2dd8
      std::array<char, 7>  text_2ddf; // DS:2ddf
      std::array<char, 6>  text_2de6; // DS:2de6
      std::array<char, 6>  text_2dec; // DS:2dec
      std::array<char, 10> text_2df2; // DS:2df2
      std::array<char, 7>  text_2dfc; // DS:2dfc
      std::array<char, 5>  text_2e03; // DS:2e03
      std::array<char, 7>  text_2e08; // DS:2e08
      std::array<char, 7>  text_2e0f; // DS:2e0f
      std::array<char, 7>  text_2e16; // DS:2e16
      std::array<char, 5>  text_2e1d; // DS:2e1d
      std::array<char, 5>  text_2e22; // DS:2e22
      std::array<char, 7>  text_2e27; // DS:2e27
      std::array<char, 7>  text_2e2e; // DS:2e2e
      std::array<char, 7>  text_2e35; // DS:2e35
      std::array<char, 5>  text_2e3c; // DS:2e3c
      std::array<char, 5>  text_2e41; // DS:2e41
      std::array<char, 7>  text_2e46; // DS:2e46
      std::array<char, 8>  text_2e4d; // DS:2e4d
      std::array<char, 7>  text_2e55; // DS:2e55
      std::array<char, 7>  text_2e5c; // DS:2e5c
      std::array<char, 7>  text_2e63; // DS:2e63
      std::array<char, 7>  text_2e6a; // DS:2e6a
      std::array<char, 7>  text_2e71; // DS:2e71
      std::array<char, 5>  text_2e78; // DS:2e78
      std::array<char, 5>  text_2e7d; // DS:2e7d
      std::array<char, 5>  text_2e82; // DS:2e82
      std::array<char, 5>  text_2e87; // DS:2e87
    };
  };
  union {
    std::array<std::uint8_t, 226> resource_filenames; // DS:2e8c..2f6e
    struct {
      std::array<char, 8>  scenario_filename; // DS:2e8c
      std::array<char, 9>  face_file_name;    // DS:2e94
      std::uint8_t         unknown_2e9d;      // DS:2e9d
      std::array<char, 11> end_data_name;     // DS:2e9e
      std::uint8_t         unknown_2ea9;      // DS:2ea9
      std::array<char, 13> text_2eaa;         // DS:2eaa
      std::uint8_t         unknown_2eb7;      // DS:2eb7
      std::array<char, 11> scensel_file;      // DS:2eb8
      std::uint8_t         unknown_2ec3;      // DS:2ec3
      std::array<char, 11> mgamen_filename;   // DS:2ec4
      std::uint8_t         unknown_2ecf;      // DS:2ecf
      std::array<char, 9>  arms_file_name;    // DS:2ed0
      std::uint8_t         unknown_2ed9;      // DS:2ed9
      std::array<char, 11> midmap_filename;   // DS:2eda
      std::uint8_t         unknown_2ee5;      // DS:2ee5
      std::array<char, 11> bigmap_filename;   // DS:2ee6
      std::uint8_t         unknown_2ef1;      // DS:2ef1
      std::array<char, 11> map_filename;      // DS:2ef2
      std::uint8_t         unknown_2efd;      // DS:2efd
      std::array<char, 11> mappno_filename;   // DS:2efe
      std::uint8_t         unknown_2f09;      // DS:2f09
      std::array<char, 11> hexmap_filename;   // DS:2f0a
      std::uint8_t         unknown_2f15;      // DS:2f15
      std::array<char, 12> hexdisp_filename;  // DS:2f16
      std::array<char, 12> hexunit_filename;  // DS:2f22
      std::array<char, 12> hexfont_filename;  // DS:2f2e
      std::array<char, 13> hexfonts_filename; // DS:2f3a
      std::uint8_t         unknown_2f47;      // DS:2f47
      std::array<char, 10> msgrp_filename;    // DS:2f48
      std::array<char, 10> event_file_name;   // DS:2f52
      std::array<char, 9>  text_2f5c;         // DS:2f5c
      std::uint8_t         unknown_2f65;      // DS:2f65
      std::array<char, 8>  text_2f66;         // DS:2f66
    };
  };
  union {
    std::array<std::uint8_t, 450> turn_variables; // DS:2f6e..3130
    struct {
      std::uint16_t picture_handle; // DS:2f6e
      union {                       // DS:2f70
        std::uint16_t               portrait_handle;
        std::array<std::uint8_t, 2> portrait_handle_bytes;
      };
      std::uint16_t                face_handle;            // DS:2f72
      std::uint16_t                message_picture_handle; // DS:2f74
      std::array<std::uint8_t, 12> unknown_2f76;
      std::uint8_t                 outcome_side;      // DS:2f82
      std::uint8_t                 outcome_reason;    // DS:2f83
      std::uint16_t                daily_pending;     // DS:2f84
      std::uint16_t                daily_active;      // DS:2f86
      std::uint16_t                side_initializing; // DS:2f88
      std::uint16_t                unit_order_active; // DS:2f8a
      std::array<std::uint8_t, 2>  schedule_flags;    // DS:2f8c
      union {                                         // DS:2f8e
        std::uint16_t               turn_notification;
        std::array<std::uint8_t, 2> turn_notification_bytes;
      };
      std::uint8_t overview_side; // DS:2f90: the side the strategic overview shows, 255 for none
      std::uint8_t view_side;     // DS:2f91: the side the tactical view shows, 255 while the overview is up
      union {                        // DS:2f92
        std::uint16_t               viewport_x;
        std::array<std::uint8_t, 2> viewport_x_bytes;
      };
      union { // DS:2f94
        std::uint16_t               viewport_y;
        std::array<std::uint8_t, 2> viewport_y_bytes;
      };
      std::uint8_t                turn_frame_mode; // DS:2f96
      std::array<std::uint8_t, 1> unknown_2f97;
      std::uint16_t               control_selection;    // DS:2f98
      std::uint16_t               context_display_word; // DS:2f9a
      std::uint8_t                context_side; // DS:2f9c: the side whose context the panel shows, 0xff for none
      std::array<std::uint8_t, 1> unknown_2f9d;
      std::uint16_t               restart_flag;         // DS:2f9e
      std::uint16_t               presentation_mode;    // DS:2fa0
      union {                                           // DS:2fa2
        std::uint8_t battle_map_x;
        std::uint8_t battle_x;
      };
      union { // DS:2fa3
        std::uint8_t battle_map_y;
        std::uint8_t battle_y;
      };
      std::uint8_t  battle_attacker_x; // DS:2fa4
      std::uint8_t  battle_attacker_y; // DS:2fa5
      std::uint8_t  battle_direction;  // DS:2fa6
      std::uint8_t  battle_side;       // DS:2fa7
      std::uint16_t battle_command[2]; // DS:2fa8
      std::uint16_t battle_report;     // DS:2fac
      union {                          // DS:2fae
        std::uint16_t battle_lists[22];
        std::uint16_t battle_units[22];
      };
      std::array<std::uint8_t, 8> overview_objective_glyph; // DS:2fda
      std::array<std::uint8_t, 8> flash_marker;             // DS:2fe2
      std::uint16_t               face_offsets[159];        // DS:2fea
      std::array<std::uint8_t, 8> ai_unit_element_counts;   // DS:3128
    };
  };
  std::uint16_t          request_capacities[8]; // DS:3130
  std::uint16_t          capacity_a_weights[8]; // DS:3140
  std::uint16_t          capacity_c_weights[8]; // DS:3150
  std::array<Person, 12> person_templates;      // DS:3160
  union {
    std::array<std::uint8_t, 94> template_tables; // DS:32d4..3332
    struct {
      std::array<std::uint8_t, 6>  weather_obscurity; // DS:32d4: by weather, how much it hinders battle fire
      std::array<std::uint8_t, 24> air_cover;     // DS:32da
      std::array<std::uint8_t, 14> battle_levels; // DS:32f2
      std::array<char, 14>         text_3300;     // DS:3300
      std::array<char, 12>         text_330e;     // DS:330e
      std::array<char, 12>         text_331a;     // DS:331a
      std::array<char, 11>         text_3326;     // DS:3326
      std::uint8_t                 unknown_3331;  // DS:3331
    };
  };
  union { // DS:3332
    std::array<DailyRecord, 158> daily_records;
    std::array<ElementType, 158> element_types;
  };
  std::uint16_t weather_names[6]; // DS:3b38
  union {                         // DS:3b44
    std::uint16_t ui_side_names[2];
    std::uint16_t side_names[2];
  };
  std::uint16_t mask_names[6];  // DS:3b48
  std::uint16_t role_names[7];  // DS:3b54
  std::uint16_t entry_names[8]; // DS:3b62
  union {                       // DS:3b72
    std::uint16_t cursor_code_text[11];
    std::uint16_t terrain_names[11];
  };
  std::uint16_t order_names[17]; // DS:3b88
  std::uint16_t supply_names[3]; // DS:3baa
  union {
    std::array<std::uint8_t, 702> small_glyphs; // DS:3bb0..3e6e
    struct {
      union { // DS:3bb0
        std::array<std::uint8_t, 66> battle_digits;
        std::array<std::uint8_t, 66> digit_glyphs; // ten 6 x 6 digits, then a blank
      };
      std::array<std::uint8_t, 56> rotated_glyphs; // DS:3bf2: ten 8 x 5 digits on their side, then a blank
      std::array<char, 5>          text_3c2a;           // DS:3c2a
      std::array<char, 7>          text_3c2f;           // DS:3c2f
      std::array<char, 6>          text_3c36;           // DS:3c36
      std::array<char, 9>          text_3c3c;           // DS:3c3c
      std::array<char, 7>          text_3c45;           // DS:3c45
      std::array<char, 5>          text_3c4c;           // DS:3c4c
      std::array<char, 5>          text_3c51;           // DS:3c51
      std::array<char, 6>          text_3c56;           // DS:3c56
      std::array<char, 7>          text_3c5c;           // DS:3c5c
      std::array<char, 6>          text_3c63;           // DS:3c63
      std::array<char, 6>          text_3c69;           // DS:3c69
      std::array<char, 6>          text_3c6f;           // DS:3c6f
      std::array<char, 7>          text_3c75;           // DS:3c75
      std::array<char, 6>          text_3c7c;           // DS:3c7c
      std::array<char, 7>          text_3c82;           // DS:3c82
      std::array<char, 9>          text_3c89;           // DS:3c89
      std::array<char, 7>          text_3c92;           // DS:3c92
      std::uint8_t                 battle_ai_side;      // DS:3c99
      std::uint8_t                 battle_active_side;  // DS:3c9a
      std::uint8_t                 battle_active_group; // DS:3c9b
      std::uint8_t                 battle_ai_index;     // DS:3c9c
      std::uint8_t                 deploy_row;          // DS:3c9d
      union {                                           // DS:3c9e
        std::uint16_t battle_hq_intact[2];
        std::uint16_t battle_panel_shown[2];
      };
      // DS:3ca2: the battle map, 14 x 14 cells by row: the low two bits are the side occupying the cell + 1, the high
      // nibble the direction + 1 of the map edge a side enters or leaves by (BattleCell)
      std::array<std::array<std::uint8_t, 14>, 14> battle_cells;
      std::uint8_t                 battle_numbers_shown; // DS:3d66
      std::array<std::uint8_t, 1>  unknown_3d67;
      std::array<std::uint8_t, 18> battle_ai_plans;       // DS:3d68
      std::uint16_t                battle_ai_targets[18]; // DS:3d7a
      std::uint8_t                 battle_level_limit;    // DS:3d9e
      std::array<std::uint8_t, 1>  unknown_3d9f;
      std::uint16_t                battle_selected; // DS:3da0
      union {                                       // DS:3da2
        std::uint16_t               battle_element_destroyed;
        std::array<std::uint8_t, 2> battle_element_destroyed_bytes;
      };
      std::uint16_t battle_lost_element; // DS:3da4
      std::uint16_t battle_lost_unit;    // DS:3da6
      union {                            // DS:3da8
        std::array<std::uint8_t, 2> battle_ai_entry_side;
        std::array<std::uint8_t, 2> battle_exit_direction;
      };
      std::uint16_t                battle_side_routed[2]; // DS:3daa
      std::array<std::uint8_t, 8>  battle_type_factors;   // DS:3dae
      std::array<std::uint8_t, 8>  battle_spot_range;     // DS:3db6
      std::array<std::uint8_t, 30> battle_terrain_extras; // DS:3dbe
      std::uint16_t battle_order_names[6];    // DS:3ddc
      std::uint16_t battle_terrain_names[11]; // DS:3de8: the battle terrain codes' names
      std::array<std::uint8_t, 88> battle_terrain_costs;  // DS:3dfe
      std::uint8_t                 battle_terrain_cover;  // DS:3e56
      std::uint8_t                 battle_visibility;     // DS:3e57
      std::array<std::uint8_t, 22> unknown_3e58;
    };
  };
  std::array<std::uint8_t, 44> fileio_disk_abort_message; // DS:3e6e
  union {
    std::array<std::uint8_t, 32> vgalib_data; // DS:3e9a..3eba
    struct {
      std::array<std::uint8_t, 16> unknown_3e9a;          // DS:3e9a
      std::array<std::uint8_t, 2>  vgalib_line_error;     // DS:3eaa
      std::uint8_t                 vgalib_pattern_bit;    // DS:3eac
      std::array<std::uint8_t, 7>  vgalib_pattern_on;     // DS:3ead
      std::array<std::uint8_t, 2>  vgalib_var_3eb4;       // DS:3eb4
      std::array<std::uint8_t, 2>  vgalib_decode_flag;    // DS:3eb6
      std::array<std::uint8_t, 2>  vgalib_decode_handler; // DS:3eb8
    };
  };
  union {
    std::array<std::uint8_t, 50> text_vars; // DS:3eba..3eec
    struct {
      FarPointer                  idle_hook;       // DS:3eba
      std::array<std::uint8_t, 4> unknown_3ebe;    // DS:3ebe
      FarPointer                  message_hook;    // DS:3ec2: called while a dialog message shows (0:0 for none)
      std::array<std::uint8_t, 4> unknown_3ec6;    // DS:3ec6
      std::int16_t                text_x;          // DS:3eca
      std::int16_t                text_y;          // DS:3ecc
      std::int16_t                text_left;       // DS:3ece
      std::int16_t                text_right;      // DS:3ed0
      std::int16_t                text_top;        // DS:3ed2
      std::int16_t                text_bottom;     // DS:3ed4
      std::int16_t                text_height;     // DS:3ed6
      std::int16_t                text_scroll;     // DS:3ed8
      std::uint8_t                text_paper;      // DS:3eda
      std::uint8_t                text_ink;        // DS:3edb
      std::uint8_t                text_background; // DS:3edc
      std::array<std::uint8_t, 1> unknown_3edd;
      std::int16_t                saved_x;          // DS:3ede
      std::int16_t                saved_y;          // DS:3ee0
      std::int16_t                saved_left;       // DS:3ee2
      std::uint8_t                saved_paper;      // DS:3ee4
      std::uint8_t                saved_ink;        // DS:3ee5
      std::uint8_t                saved_background; // DS:3ee6
      std::uint8_t                text_lead;        // DS:3ee7
      std::int16_t                text_state;       // DS:3ee8
      std::uint16_t               format_output;    // DS:3eea: where opeu_request_format writes next
    };
  };
  std::array<std::uint8_t, 1536> text_font; // DS:3eec
  union {
    std::array<std::uint8_t, 3211> setup_text; // DS:44ec..5177
    struct {
      std::array<FarPointer, 2>   glyph_draw; // DS:44ec: the glyph routines, transparent and opaque
      std::array<std::uint8_t, 8> text_erase;      // DS:44f4
      std::array<char, 3>         prompt_format;   // DS:44fc
      std::array<char, 3>         question_format; // DS:44ff
      union {                                      // DS:4502
        std::uint16_t               input_mode;
        std::array<std::uint8_t, 2> input_mode_bytes;
      };
      std::array<char, 13>        menu_format;             // DS:4504
      std::uint8_t                unknown_4511;            // DS:4511
      std::uint16_t               menu_save_offset;        // DS:4512
      std::uint16_t               numdlg_x;                // DS:4514
      std::uint16_t               numdlg_y;                // DS:4516
      std::array<std::uint8_t, 5> numdlg_digits;           // DS:4518
      std::array<char, 10>        numdlg_format;           // DS:451d
      std::uint8_t                unknown_4527;            // DS:4527
      std::array<char, 8>         ems_device_name;         // DS:4528
      std::uint16_t               numdlg_image_mapped;     // DS:4530
      std::uint16_t               tile_mapped;             // DS:4532
      std::uint16_t               sprite_mapped;           // DS:4534
      std::uint16_t               frame_mapped;            // DS:4536
      std::uint16_t               setup_marker_points[14]; // DS:4538
      std::uint16_t               mgamen_blocks[6];        // DS:4554
      std::array<char, 9>         savedata_name;           // DS:4560
      std::array<char, 26>        no_saved_games_text;     // DS:4569
      std::array<char, 3>         load_print_format;       // DS:4583
      std::array<char, 9>         load_savedata_name;      // DS:4586
      std::array<char, 17>        load_which_text;         // DS:458f
      std::array<char, 22>        loading_text;            // DS:45a0
      std::array<char, 28>        loaded_text;             // DS:45b6
      std::array<char, 29>        load_failed_text;        // DS:45d2
      std::array<char, 8>         text_45ef;               // DS:45ef
      std::array<char, 9>         text_45f7;               // DS:45f7
      union {                                              // DS:4600
        std::uint16_t               shown_portrait;
        std::array<std::uint8_t, 2> shown_portrait_bytes;
      };
      ListWindow                   play_dialog;                // DS:4602: the generals' list of the play screen
      std::uint8_t                 bytes_4615;                 // DS:4615
      ListWindow                   reinforce_dialog;           // DS:4616: the generals' list of the reinforcement screen
      std::array<char, 10>         division_reserve_mark;      // DS:4629
      std::uint8_t                 empty_suffix;               // DS:4633
      std::array<char, 31>         division_strength_format;   // DS:4634
      std::array<char, 16>         chief_ops_format;           // DS:4653
      std::array<char, 38>         chief_ops_prompt;           // DS:4663
      std::array<char, 16>         chief_intel_format;         // DS:4689
      std::array<char, 39>         chief_intel_prompt;         // DS:4699
      std::array<char, 16>         division_format;            // DS:46c0
      std::array<char, 14>         division_short_format;      // DS:46d0
      std::array<char, 16>         division_long_format;       // DS:46de
      std::array<char, 16>         role_prompt;                // DS:46ee
      std::array<char, 30>         division_prompt;            // DS:46fe
      std::array<char, 41>         general_header;             // DS:471c
      std::array<char, 9>          general_name_format;        // DS:4745
      std::array<char, 46>         general_stats_format;       // DS:474e
      std::array<char, 14>         text_477c;                  // DS:477c
      std::array<char, 16>         text_478a;                  // DS:478a
      std::array<char, 9>          side_title_format;          // DS:479a
      std::uint8_t                 unknown_47a3;               // DS:47a3
      std::uint16_t                side_titles[2];             // DS:47a4
      std::array<std::uint8_t, 23> reinforce_title;            // DS:47a8
      std::array<char, 90>         reinforce_mouse_help;       // DS:47bf
      std::array<char, 87>         reinforce_key_help;         // DS:4819
      std::array<char, 20>         play_prompt;                // DS:4870
      std::array<char, 20>         use_generals_prompt;        // DS:4884
      std::array<char, 16>         scenario_message_format;    // DS:4898
      std::array<std::uint8_t, 2>  unknown_48a8;               // DS:48a8
      std::array<char, 10>         text_48aa;                  // DS:48aa
      std::array<char, 12>         text_48b4;                  // DS:48b4
      std::array<char, 11>         text_48c0;                  // DS:48c0
      std::array<char, 12>         text_48cb;                  // DS:48cb
      std::array<char, 13>         text_48d7;                  // DS:48d7
      std::array<char, 12>         text_48e4;                  // DS:48e4
      std::array<char, 10>         text_48f0;                  // DS:48f0
      std::array<char, 11>         text_48fa;                  // DS:48fa
      std::array<char, 12>         text_4905;                  // DS:4905
      std::array<char, 15>         text_4911;                  // DS:4911
      std::array<char, 13>         text_4920;                  // DS:4920
      std::array<char, 6>          text_492d;                  // DS:492d
      std::array<char, 12>         text_4933;                  // DS:4933
      std::array<char, 19>         text_493f;                  // DS:493f
      std::array<char, 11>         text_4952;                  // DS:4952
      std::array<char, 6>          text_495d;                  // DS:495d
      std::array<char, 9>          text_4963;                  // DS:4963
      std::array<char, 6>          text_496c;                  // DS:496c
      std::array<char, 11>         text_4972;                  // DS:4972
      std::array<char, 20>         text_497d;                  // DS:497d
      std::array<char, 29>         text_4991;                  // DS:4991
      std::array<char, 29>         text_49ae;                  // DS:49ae
      std::array<char, 19>         text_49cb;                  // DS:49cb
      std::array<char, 25>         text_49de;                  // DS:49de
      std::array<char, 23>         text_49f7;                  // DS:49f7
      std::array<char, 10>         scensel_colors;             // DS:4a0e
      std::uint8_t                 scensel_empty;              // DS:4a18
      std::array<char, 21>         scensel_france;             // DS:4a19
      std::array<char, 26>         scensel_africa;             // DS:4a2e
      std::array<char, 18>         scensel_kursk;              // DS:4a48
      std::array<char, 17>         scensel_dday;               // DS:4a5a
      std::array<char, 22>         scensel_bulge;              // DS:4a6b
      std::array<char, 17>         scensel_berlin;             // DS:4a81
      std::array<char, 17>         scensel_demo;               // DS:4a92
      std::array<char, 20>         scensel_view_format;        // DS:4aa3
      std::array<char, 16>         scensel_view_line;          // DS:4ab7
      std::array<char, 29>         scensel_one_player;         // DS:4ac7
      std::array<char, 20>         scensel_command_format;     // DS:4ae4
      std::array<char, 19>         scensel_command_line;       // DS:4af8
      std::array<char, 30>         scensel_two_players;        // DS:4b0b
      std::array<char, 8>          scensel_aerial;             // DS:4b29
      std::array<char, 8>          scensel_field;              // DS:4b31
      std::array<char, 21>         scensel_map_format;         // DS:4b39
      std::array<char, 19>         scensel_map_line;           // DS:4b4e
      std::array<char, 23>         scensel_view_all;           // DS:4b61
      std::array<char, 23>         scensel_view_none;          // DS:4b78
      std::array<char, 23>         scensel_view_selected;      // DS:4b8f
      std::array<char, 19>         scensel_battles_line;       // DS:4ba6
      std::array<char, 19>         scensel_difficulty;         // DS:4bb9
      std::array<char, 10>         scensel_very_easy;          // DS:4bcc
      std::array<char, 10>         scensel_easy;               // DS:4bd6
      std::array<char, 10>         scensel_average;            // DS:4be0
      std::array<char, 10>         scensel_hard;               // DS:4bea
      std::array<char, 10>         scensel_very_hard;          // DS:4bf4
      std::array<char, 19>         scensel_difficulty_line;    // DS:4bfe
      std::array<char, 21>         scensel_settings_ok;        // DS:4c11
      std::array<char, 34>         scensel_reselect;           // DS:4c26
      std::array<char, 35>         scensel_ready_demo;         // DS:4c48
      std::array<char, 7>          scensel_ready_colors;       // DS:4c6b
      std::array<char, 7>          scensel_load_colors;        // DS:4c72
      std::array<char, 29>         scensel_ready_blank;        // DS:4c79
      std::array<char, 27>         scensel_ready;              // DS:4c96
      std::array<char, 10>         scensel_done_colors;        // DS:4cb1
      std::uint8_t                 unknown_4cbb;               // DS:4cbb
      std::uint16_t                scensel_start_menu[4];      // DS:4cbc
      std::uint16_t                near_4cc4;                  // DS:4cc4
      std::uint16_t                words_4cc6[5];              // DS:4cc6
      std::uint16_t                near_4cd0;                  // DS:4cd0
      std::array<std::uint8_t, 14> bytes_4cd2;                 // DS:4cd2
      std::array<char, 32>         scensel_start_prompt;       // DS:4ce0
      std::uint16_t                scensel_players_menu[4];    // DS:4d00
      std::uint16_t                near_4d08;                  // DS:4d08
      std::uint16_t                words_4d0a[5];              // DS:4d0a
      std::uint16_t                near_4d14;                  // DS:4d14
      std::uint16_t                words_4d16[5];              // DS:4d16
      std::uint16_t                near_4d20;                  // DS:4d20
      std::array<std::uint8_t, 14> bytes_4d22;                 // DS:4d22
      std::array<char, 24>         scensel_players_prompt;     // DS:4d30
      std::array<char, 32>         scensel_view_prompt;        // DS:4d48
      std::uint16_t                scensel_side_menu[4];       // DS:4d68
      std::uint16_t                near_4d70;                  // DS:4d70
      std::uint16_t                words_4d72[5];              // DS:4d72
      std::uint16_t                near_4d7c;                  // DS:4d7c
      std::array<std::uint8_t, 14> bytes_4d7e;                 // DS:4d7e
      std::array<char, 35>         scensel_command_prompt;     // DS:4d8c
      std::uint8_t                 unknown_4daf;               // DS:4daf
      std::uint16_t                scensel_map_menu[4];        // DS:4db0
      std::uint16_t                near_4db8;                  // DS:4db8
      std::uint16_t                words_4dba[5];              // DS:4dba
      std::uint16_t                near_4dc4;                  // DS:4dc4
      std::array<std::uint8_t, 14> bytes_4dc6;                 // DS:4dc6
      std::array<char, 30>         scensel_map_prompt;         // DS:4dd4
      std::uint16_t                scensel_battles_menu[4];    // DS:4df2
      std::uint16_t                near_4dfa;                  // DS:4dfa
      std::uint16_t                words_4dfc[5];              // DS:4dfc
      std::uint16_t                near_4e06;                  // DS:4e06
      std::array<std::uint8_t, 14> bytes_4e08;                 // DS:4e08
      std::array<char, 21>         scensel_battles_prompt;     // DS:4e16
      std::uint8_t                 unknown_4e2b;               // DS:4e2b
      std::uint16_t                scensel_battles_menu2[4];   // DS:4e2c
      std::uint16_t                near_4e34;                  // DS:4e34
      std::uint16_t                words_4e36[5];              // DS:4e36
      std::uint16_t                near_4e40;                  // DS:4e40
      std::uint16_t                words_4e42[5];              // DS:4e42
      std::uint16_t                near_4e4c;                  // DS:4e4c
      std::array<std::uint8_t, 14> bytes_4e4e;                 // DS:4e4e
      std::array<char, 21>         scensel_battles_prompt2;    // DS:4e5c
      std::uint8_t                 unknown_4e71;               // DS:4e71
      std::uint16_t                scensel_difficulty_menu[4]; // DS:4e72
      std::uint16_t                near_4e7a;                  // DS:4e7a
      std::uint16_t                words_4e7c[5];              // DS:4e7c
      std::uint16_t                near_4e86;                  // DS:4e86
      std::uint16_t                words_4e88[5];              // DS:4e88
      std::uint16_t                near_4e92;                  // DS:4e92
      std::uint16_t                words_4e94[5];              // DS:4e94
      std::uint16_t                near_4e9e;                  // DS:4e9e
      std::array<std::uint8_t, 10> bytes_4ea0;                 // DS:4ea0
      std::uint16_t                near_4eaa;                  // DS:4eaa
      std::array<std::uint8_t, 14> bytes_4eac;                 // DS:4eac
      std::array<char, 30>         scensel_difficulty_prompt;  // DS:4eba
      std::uint16_t                scensel_scenario_menu[4];   // DS:4ed8
      std::uint16_t                near_4ee0;                  // DS:4ee0
      std::uint16_t                words_4ee2[5];              // DS:4ee2
      std::uint16_t                near_4eec;                  // DS:4eec
      std::uint16_t                words_4eee[5];              // DS:4eee
      std::uint16_t                near_4ef8;                  // DS:4ef8
      std::uint16_t                words_4efa[5];              // DS:4efa
      std::uint16_t                near_4f04;                  // DS:4f04
      std::uint16_t                words_4f06[5];              // DS:4f06
      std::uint16_t                near_4f10;                  // DS:4f10
      std::array<std::uint8_t, 10> bytes_4f12;                 // DS:4f12
      std::uint16_t                near_4f1c;                  // DS:4f1c
      std::array<std::uint8_t, 14> bytes_4f1e;                 // DS:4f1e
      std::array<char, 36>         scensel_scenario_prompt;    // DS:4f2c
      std::uint16_t                scensel_pictures[7];        // DS:4f50
      std::array<char, 67>         memory_message;             // DS:4f5e
      std::array<char, 72>         memory_advice;              // DS:4fa1
      std::array<char, 28>         memory_exit;                // DS:4fe9
      std::array<char, 8>          setup_name_bmapseg;         // DS:5005
      std::array<char, 9>          setup_name_hunitseg;        // DS:500d
      std::array<char, 7>          setup_name_grpseg;          // DS:5016
      std::array<char, 10>         setup_name_ms_winseg;       // DS:501d
      std::array<char, 5>          setup_name_vseg;            // DS:5027
      std::array<char, 9>          setup_name_hdispseg;        // DS:502c
      std::array<char, 8>          setup_name_armsseg;         // DS:5035
      std::array<char, 7>          setup_name_kaoseg;          // DS:503d
      std::array<char, 9>          setup_name_mdispseg;        // DS:5044
      std::array<char, 7>          setup_name_ms_seg;          // DS:504d
      std::array<char, 9>          setup_name_hfontseg;        // DS:5054
      std::array<char, 8>          setup_name_mmapseg;         // DS:505d
      std::uint8_t                 unknown_5065;               // DS:5065
      std::array<std::uint8_t, 48> end_palette;                // DS:5066
      std::array<std::uint8_t, 43> end_palette_final;          // DS:5096
      std::array<char, 5>          text_50c1;                  // DS:50c1
      std::array<std::uint8_t, 12> end_text_format;            // DS:50c6
      std::array<char, 11>         end_picture_name;           // DS:50d2
      std::array<char, 27>         end_salute;                 // DS:50dd
      std::array<FarPointer, 4>    end_victory_texts; // DS:50f8: by elimination, then by rank 1..3
      std::array<FarPointer, 4>    end_defeat_texts;  // DS:5108: by the outcome reason
      std::array<char, 8>          name_bmapseg;               // DS:5118
      std::array<char, 8>          name_mmapseg;               // DS:5120
      std::array<char, 9>          name_hdispseg;              // DS:5128
      std::array<char, 9>          name_hunitseg;              // DS:5131
      std::array<char, 7>          name_grpseg;                // DS:513a
      std::array<char, 10>         name_ms_winseg;             // DS:5141
      std::array<char, 7>          name_ms_seg;                // DS:514b
      std::array<char, 9>          name_hfontseg;              // DS:5152
      std::array<char, 5>          name_vseg;                  // DS:515b
      std::array<char, 7>          name_kaoseg;                // DS:5160
      std::array<char, 8>          name_armsseg;               // DS:5167
      std::array<std::uint8_t, 8>  name_mdispseg;              // DS:516f
    };
  };
  std::array<std::uint8_t, 13> calendar_days; // DS:5177
  union {
    std::array<std::uint8_t, 488> startup_text; // DS:5184..536c
    struct {
      char                         startup_mode;           // DS:5184
      std::array<std::uint8_t, 59> input_prompt;           // DS:5185
      std::uint16_t                stream_count;           // DS:51c0
      std::array<CacheNode, 10>    portrait_nodes;         // DS:51c2
      std::uint16_t                portrait_head;          // DS:51d6: near pointer to the most recent node
      std::array<CacheNode, 10>    face_nodes;             // DS:51d8
      std::uint16_t                face_head;              // DS:51ec: near pointer to the most recent node
      std::array<std::uint8_t, 32> backdrop_pattern;       // DS:51ee
      std::array<std::uint8_t, 32> backdrop_color_pattern; // DS:520e
      std::array<std::uint8_t, 24> palette_base;           // DS:522e
      std::array<std::uint8_t, 24> palette_dawn;           // DS:5246
      std::array<std::uint8_t, 24> palette_day;            // DS:525e
      std::array<std::uint8_t, 24> palette_dusk;           // DS:5276
      std::array<std::uint8_t, 24> palette_night;          // DS:528e
      std::array<std::uint8_t, 48> palette_title;          // DS:52a6
      std::array<std::uint8_t, 43> palette_map;            // DS:52d6
      std::array<char, 5>          text_5301;              // DS:5301
      std::array<std::uint8_t, 48> general_stats_line;     // DS:5306
      std::array<char, 54>         div_commander_prompt;   // DS:5336
    };
  };
  union {
    std::array<std::uint8_t, 16> step_data; // DS:536c..537c
    struct {
      std::array<std::uint8_t, 8> step_biased_dx; // DS:536c
      std::array<std::uint8_t, 8> step_biased_dy; // DS:5374
    };
  };
  union {
    std::array<std::uint8_t, 2102> label_text; // DS:537c..5bb2
    struct {
      std::array<char, 7>          unit_name_format;       // DS:537c
      std::array<char, 6>          type_name_suffix;       // DS:5383
      std::uint8_t                 item_name_suffix;       // DS:5389
      std::array<char, 8>          location_name_format;   // DS:538a
      std::array<std::uint8_t, 4>  deputy_identity_ranges; // DS:5392
      std::array<char, 12>         deputy_name;            // DS:5396
      std::array<char, 27>         army_out;               // DS:53a2
      std::array<char, 52>         army_row0;              // DS:53bd
      std::array<char, 49>         army_row1;              // DS:53f1
      std::array<char, 49>         army_row2;              // DS:5422
      std::array<char, 33>         army_row3;              // DS:5453
      std::array<char, 39>         army_row4;              // DS:5474
      std::array<char, 37>         army_row5;              // DS:549b
      std::array<char, 20>         army_hq;                // DS:54c0
      std::array<char, 12>         army_person_format;     // DS:54d4
      std::array<char, 8>          army_hq_label;          // DS:54e0
      std::array<char, 15>         division_label_format;  // DS:54e8
      std::array<char, 12>         division_person_format; // DS:54f7
      std::array<char, 6>          panel_name_format;      // DS:5503
      std::array<char, 11>         panel_count_format;     // DS:5509
      std::array<char, 8>          text_5514;              // DS:5514
      std::array<char, 8>          text_551c;              // DS:551c
      std::array<char, 9>          text_5524;              // DS:5524
      std::array<char, 7>          text_552d;              // DS:552d
      std::array<char, 17>         panel_regiment_format;  // DS:5534
      std::array<char, 7>          panel_battle;           // DS:5545
      std::array<char, 3>          panel_string_format;    // DS:554c
      std::uint8_t                 unknown_554f;           // DS:554f
      std::array<std::uint16_t, 4> panel_regiment_names;   // DS:5550: the names of regiments 5-8 of a division
      std::array<std::uint8_t, 36> credits_title;          // DS:5558
      std::array<char, 28>         credits_main;           // DS:557c
      std::array<char, 33>         credits_support;        // DS:5598
      std::array<char, 58>         credits_translation;    // DS:55b9
      std::uint8_t                 unknown_55f3;           // DS:55f3
      std::array<char, 38>         tbl_group_header;       // DS:55f4
      std::array<char, 3>          tbl_group_title;        // DS:561a
      std::array<char, 4>          tbl_group_mark;         // DS:561d
      std::array<char, 18>         tbl_group_division;     // DS:5621
      std::array<char, 10>         tbl_group_empty;        // DS:5633
      std::array<char, 6>          tbl_group_div;          // DS:563d
      std::array<char, 7>          tbl_group_army;         // DS:5643
      std::array<char, 38>         tbl_group_row;          // DS:564a
      std::array<char, 39>         tbl_group_blank;        // DS:5670
      std::array<char, 7>          text_5697;              // DS:5697
      std::array<char, 10>         tbl_mission_header;     // DS:569e
      std::array<char, 14>         tbl_mission_label;      // DS:56a8
      std::array<char, 17>         tbl_mission_place;      // DS:56b6
      std::array<char, 10>         tbl_mission_target;     // DS:56c7
      std::array<char, 8>          tbl_mission_row;        // DS:56d1
      std::array<char, 38>         tbl_mission_blank;      // DS:56d9
      std::uint8_t                 unknown_56ff;           // DS:56ff
      ListWindow division_dialog; // DS:5700: the list of the side's divisions
      std::array<std::uint8_t, 3> bytes_5713; // DS:5713
      std::array<char, 47>         tbl_unit_header;    // DS:5716
      std::array<char, 4>          tbl_unit_mark;      // DS:5745
      std::array<char, 11>         tbl_unit_label;     // DS:5749
      std::array<char, 13>         tbl_unit_empty;     // DS:5754
      std::array<char, 48>         tbl_unit_blank;     // DS:5761
      std::array<char, 5>          tbl_unit_wait;      // DS:5791
      std::array<char, 42>         tbl_unit_row;       // DS:5796
      std::array<char, 45>         tbl_unit_hidden;    // DS:57c0
      std::uint8_t                 unknown_57ed;       // DS:57ed
      ListWindow                   order_target_list;  // DS:57ee: the list of a division's objective tasks
      std::array<std::uint8_t, 8>  bytes_5801;         // DS:5801
      std::array<char, 51>         tbl_order_header;   // DS:5809
      std::array<char, 11>         tbl_order_label;    // DS:583c
      std::uint8_t                 unknown_5847;       // DS:5847
      ListWindow organize_list; // DS:5848: the list of a division's regiments
      std::array<std::uint8_t, 8> bytes_585b; // DS:585b
      std::array<char, 30>         tbl_element_header; // DS:5863
      std::array<char, 4>          tbl_element_mark;   // DS:5881
      std::array<char, 10>         tbl_element_name;   // DS:5885
      std::array<char, 31>         tbl_element_row;    // DS:588f
      ListWindow                   regiment_dialog;    // DS:58ae: the regiment choice list
      std::array<std::uint8_t, 7>  bytes_58c1;         // DS:58c1
      std::array<char, 5>          text_58c8;          // DS:58c8
      std::array<char, 5>          text_58cd;          // DS:58cd
      std::array<char, 5>          text_58d2;          // DS:58d2
      std::array<char, 5>          text_58d7;          // DS:58d7
      std::array<char, 6>          text_58dc;          // DS:58dc
      std::array<char, 6>          text_58e2;          // DS:58e2
      std::array<char, 6>          text_58e8;          // DS:58e8
      std::array<char, 9>          tbl_person_name;    // DS:58ee
      std::array<char, 5>          tbl_person_blank;   // DS:58f7
      std::array<char, 5>          tbl_person_rest;    // DS:58fc
      std::array<char, 5>          tbl_person_hurt;    // DS:5901
      ListWindow                   choice_dialog;      // DS:5906: the element choice list
      std::array<std::uint8_t, 1>  bytes_5919;         // DS:5919
      std::uint16_t                tbl_post_names[7];  // DS:591a: Div1..Div4, then the staff posts Cmdr, Ops, Intl
      std::array<std::uint8_t, 13> bytes_5928;         // DS:5928
      std::array<char, 39>         tbl_type_header;    // DS:5935
      std::array<char, 9>          tbl_type_name;      // DS:595c
      std::array<char, 37>         tbl_type_row;       // DS:5965
      ListWindow people_dialog; // DS:598a: the list of generals
      std::array<char, 8>          text_599d;         // DS:599d
      std::array<std::uint8_t, 48> tbl_supply_header; // DS:59a5
      std::array<char, 9>          tbl_supply_name;   // DS:59d5
      std::array<char, 47>         tbl_supply_row;    // DS:59de
      std::array<char, 48>         tbl_supply_blank;  // DS:5a0d
      std::uint8_t                 unknown_5a3d;      // DS:5a3d
      ListWindow weapons_dialog; // DS:5a3e: the list of equipment types
      std::array<std::uint8_t, 6> bytes_5a51; // DS:5a51
      std::array<char, 13>         tbl_str_header;    // DS:5a57
      std::array<char, 8>          tbl_str_name;      // DS:5a64
      std::array<char, 11>         tbl_str_row;       // DS:5a6c
      std::uint8_t                 unknown_5a77;      // DS:5a77
      ListWindow                   city_dialog; // DS:5a78: the list of the side's cities (core/vwcity, core/stockord)
      std::array<std::uint8_t, 11> bytes_5a8b;  // DS:5a8b
      ListWindow stock_dialog; // DS:5a96: the list of a city's stocks
      std::uint8_t unknown_5aa9; // DS:5aa9
      std::array<std::uint8_t, 11> save_menu;           // DS:5aaa
      FarPointer                   far_code_5ab5;       // DS:5ab5
      FarPointer                   far_code_5ab9;       // DS:5ab9
      std::array<char, 9>          text_5abd;           // DS:5abd
      std::array<char, 9>          text_5ac6;           // DS:5ac6
      std::array<char, 54>         tbl_save_header;     // DS:5acf
      std::array<char, 7>          tbl_save_number;     // DS:5b05
      std::array<char, 7>          tbl_map_france;      // DS:5b0c
      std::array<char, 7>          tbl_map_africa;      // DS:5b13
      std::array<char, 7>          tbl_map_kursk;       // DS:5b1a
      std::array<char, 7>          tbl_map_dday;        // DS:5b21
      std::array<char, 7>          tbl_map_bulge;       // DS:5b28
      std::array<char, 7>          tbl_map_berlin;      // DS:5b2f
      std::array<char, 7>          tbl_level_veasy;     // DS:5b36
      std::array<char, 7>          tbl_level_easy;      // DS:5b3d
      std::array<char, 7>          tbl_level_avg;       // DS:5b44
      std::array<char, 7>          tbl_level_hard;      // DS:5b4b
      std::array<char, 7>          tbl_level_vhard;     // DS:5b52
      std::array<char, 42>         tbl_save_row;        // DS:5b59
      std::uint8_t                 unknown_5b83;        // DS:5b83
      std::uint16_t                tbl_player_names[2]; // DS:5b84
      std::uint8_t                 music_current;       // DS:5b88
      std::uint8_t                 effect_current;      // DS:5b89
      std::array<std::uint8_t, 26> music_bank;          // DS:5b8a
      std::array<std::uint8_t, 14> music_path;          // DS:5ba4
    };
  };
  union {
    std::array<std::uint8_t, 6056> briefing_text; // DS:5bb2..735a
    struct {
      std::array<char, 97> text_5bb2; // DS:5bb2
      std::array<char, 57> text_5c13; // DS:5c13
      std::array<char, 56> text_5c4c; // DS:5c4c
      std::array<char, 39> text_5c84; // DS:5c84
      std::array<char, 51> text_5cab; // DS:5cab
      std::array<char, 62> text_5cde; // DS:5cde
      std::array<char, 66> text_5d1c; // DS:5d1c
      std::array<char, 70> text_5d5e; // DS:5d5e
      std::array<char, 58> text_5da4; // DS:5da4
      std::array<char, 55> text_5dde; // DS:5dde
      std::array<char, 65> text_5e15; // DS:5e15
      std::array<char, 45> text_5e56; // DS:5e56
      std::array<char, 38> text_5e83; // DS:5e83
      std::array<char, 50> text_5ea9; // DS:5ea9
      std::array<char, 38> text_5edb; // DS:5edb
      std::array<char, 53> text_5f01; // DS:5f01
      std::array<char, 38> text_5f36; // DS:5f36
      std::array<char, 37> text_5f5c; // DS:5f5c
      std::array<char, 47> text_5f81; // DS:5f81
      std::array<char, 43> text_5fb0; // DS:5fb0
      std::array<char, 45> text_5fdb; // DS:5fdb
      std::array<char, 54> text_6008; // DS:6008
      std::array<char, 50> text_603e; // DS:603e
      std::array<char, 69> text_6070; // DS:6070
      std::array<char, 45> text_60b5; // DS:60b5
      std::array<char, 51> text_60e2; // DS:60e2
      std::array<char, 52> text_6115; // DS:6115
      std::array<char, 54> text_6149; // DS:6149
      std::array<char, 52> text_617f; // DS:617f
      std::array<char, 35> text_61b3; // DS:61b3
      std::array<char, 70> text_61d6; // DS:61d6
      std::array<char, 71> text_621c; // DS:621c
      std::array<char, 64> text_6263; // DS:6263
      std::array<char, 50> text_62a3; // DS:62a3
      std::array<char, 43> text_62d5; // DS:62d5
      std::array<char, 48> text_6300; // DS:6300
      std::array<char, 61> text_6330; // DS:6330
      std::array<char, 64> text_636d; // DS:636d
      std::array<char, 58> text_63ad; // DS:63ad
      std::array<char, 57> text_63e7; // DS:63e7
      std::array<char, 52> text_6420; // DS:6420
      std::array<char, 62> text_6454; // DS:6454
      std::array<char, 59> text_6492; // DS:6492
      std::array<char, 58> text_64cd; // DS:64cd
      std::array<char, 45> text_6507; // DS:6507
      std::array<char, 55> text_6534; // DS:6534
      std::array<char, 50> text_656b; // DS:656b
      std::array<char, 58> text_659d; // DS:659d
      std::array<char, 59> text_65d7; // DS:65d7
      std::array<char, 39> text_6612; // DS:6612
      std::array<char, 31> text_6639; // DS:6639
      std::array<char, 52> text_6658; // DS:6658
      std::array<char, 63> text_668c; // DS:668c
      std::array<char, 61> text_66cb; // DS:66cb
      std::array<char, 66> text_6708; // DS:6708
      std::array<char, 54> text_674a; // DS:674a
      std::array<char, 55> text_6780; // DS:6780
      std::array<char, 49> text_67b7; // DS:67b7
      std::array<char, 66> text_67e8; // DS:67e8
      std::array<char, 65> text_682a; // DS:682a
      std::array<char, 75> text_686b; // DS:686b
      std::array<char, 23> text_68b6; // DS:68b6
      std::array<char, 50> text_68cd; // DS:68cd
      std::array<char, 59> text_68ff; // DS:68ff
      std::array<char, 72> text_693a; // DS:693a
      std::array<char, 89> text_6982; // DS:6982
      std::array<char, 57> text_69db; // DS:69db
      std::array<char, 72> text_6a14; // DS:6a14
      std::array<char, 53> text_6a5c; // DS:6a5c
      std::array<char, 68> text_6a91; // DS:6a91
      std::array<char, 69> text_6ad5; // DS:6ad5
      std::array<char, 56> text_6b1a; // DS:6b1a
      std::array<char, 77> text_6b52; // DS:6b52
      std::array<char, 70> text_6b9f; // DS:6b9f
      std::array<char, 50> text_6be5; // DS:6be5
      std::array<char, 57> text_6c17; // DS:6c17
      std::array<char, 91> text_6c50; // DS:6c50
      std::array<char, 59> text_6cab; // DS:6cab
      std::array<char, 74> text_6ce6; // DS:6ce6
      std::array<char, 34> text_6d30; // DS:6d30
      std::array<char, 61> text_6d52; // DS:6d52
      std::array<char, 39> text_6d8f; // DS:6d8f
      std::array<char, 57> text_6db6; // DS:6db6
      std::array<char, 66> text_6def; // DS:6def
      std::array<char, 69> text_6e31; // DS:6e31
      std::array<char, 49> text_6e76; // DS:6e76
      std::array<char, 46> text_6ea7; // DS:6ea7
      std::array<char, 28> text_6ed5; // DS:6ed5
      std::array<char, 46> text_6ef1; // DS:6ef1
      std::array<char, 65> text_6f1f; // DS:6f1f
      std::array<char, 57> text_6f60; // DS:6f60
      std::array<char, 41> text_6f99; // DS:6f99
      std::array<char, 70> text_6fc2; // DS:6fc2
      std::array<char, 55> text_7008; // DS:7008
      std::array<char, 63> text_703f; // DS:703f
      std::array<char, 31> text_707e; // DS:707e
      std::array<char, 59> text_709d; // DS:709d
      std::array<char, 59> text_70d8; // DS:70d8
      std::array<char, 45> text_7113; // DS:7113
      std::array<char, 44> text_7140; // DS:7140
      std::array<char, 49> text_716c; // DS:716c
      std::array<char, 69> text_719d; // DS:719d
      std::array<char, 53> text_71e2; // DS:71e2
      std::array<char, 30> text_7217; // DS:7217
      std::array<char, 57> text_7235; // DS:7235
      std::array<char, 32> text_726e; // DS:726e
      std::array<char, 46> text_728e; // DS:728e
      std::array<char, 82> text_72bc; // DS:72bc
      std::array<char, 76> text_730e; // DS:730e
    };
  };
  std::array<FarPointer, 6> side_initializers; // DS:735a
  union {
    std::array<std::uint8_t, 2742> event_tables; // DS:7372..7e28
    struct {
      std::array<char, 7>          new_weapon_tank;       // DS:7372
      std::array<char, 2>          new_weapon_period;     // DS:7379
      std::array<char, 50>         new_weapon_format;     // DS:737b
      std::uint8_t                 unknown_73ad;          // DS:73ad
      std::array<char, 7>          noon_role_style;       // DS:73ae: "\033M4\033C6", the speaker's post
      std::array<char, 5>          noon_name_format;      // DS:73b5: ": %s" before the name
      std::array<char, 10>         noon_speech_style;     // DS:73ba: "\033M0\033C7\033B1", the speech
      std::array<char, 7>          noon_speech_end;       // DS:73c4: "\033C7\033B0"
      std::uint8_t                 unknown_73cb;          // DS:73cb
      std::array<FarPointer, 49>   noon_speeches;         // DS:73cc: what a leader says at a noon event, by line
      std::array<char, 3>          noon_news_end;         // DS:7490: "\033S", ends a noon news text
      std::uint8_t                 unknown_7493;          // DS:7493
      std::array<NoonEvent, 109>   noon_events;           // DS:7494: the dated noon events, ended by date 0xff
      std::array<std::uint8_t, 7>  mission_enemy_text;    // DS:79b0
      std::uint8_t                 mission_empty_text;    // DS:79b7
      std::array<char, 23>         raid_attempt_text;     // DS:79b8
      std::array<char, 13>         raid_attempt_tail;     // DS:79cf
      std::array<char, 15>         raid_unit_text;        // DS:79dc
      std::array<char, 13>         raid_unit_tail;        // DS:79eb
      std::array<char, 27>         sabotage_text;         // DS:79f8
      std::array<char, 16>         sabotage_tail;         // DS:7a13
      std::array<std::uint8_t, 2>  mission3_suffix;       // DS:7a23
      std::array<std::uint8_t, 2>  mission3_tail;         // DS:7a25
      std::array<char, 3>          mission_bold_off;      // DS:7a27
      std::array<char, 24>         recon_objective_text;  // DS:7a2a
      std::array<char, 2>          recon_newline;         // DS:7a42
      std::array<char, 16>         recon_unit_text;       // DS:7a44
      union {
        std::array<FarPointer, 8> mission_handlers; // DS:7a54: the special-mission handlers, by mission kind
        struct {
          std::array<std::uint8_t, 31> mission_handler_bytes;
          // DS:7a73: the climate of each month (1..12); [0] is the last handler's segment high byte
          std::array<std::uint8_t, 13> month_climate;
        };
      };
      std::array<char, 58>         air_postponed_text;    // DS:7a80: "Bad weather has postponed the %s"
      std::array<char, 80>         air_canceled_text;     // DS:7aba: "... has been canceled"
      std::array<char, 30>         air_destroyed_format;         // DS:7b0a
      std::array<char, 11>         air_strike_name;         // DS:7b28
      std::array<char, 22>         air_strike_begin;         // DS:7b33
      std::array<char, 30>         air_strike_spotted;         // DS:7b49
      std::array<char, 8>          air_bombers_name;         // DS:7b67
      std::array<char, 15>         paratroop_drop_name;   // DS:7b6f: "paratroop drop"
      std::array<char, 18>         paratroop_drop_begin;  // DS:7b7e: "Begin Airdrop!", our side
      std::array<char, 25>         paratroop_drop_spotted; // DS:7b90: "Enemy planes spotted!", the other side
      std::array<char, 11>         paratroop_planes_name; // DS:7ba9: "transports"
      std::array<char, 11>         cargo_drop_name;         // DS:7bb4
      std::array<char, 19>         cargo_drop_begin;         // DS:7bbf
      std::array<char, 26>         cargo_drop_spotted;         // DS:7bd2
      std::array<char, 13>         cargo_planes_name;         // DS:7bec
      std::array<char, 54>         cargo_unit_report;         // DS:7bf9
      std::array<char, 54>         cargo_city_report;         // DS:7c2f
      std::uint8_t                 unknown_7c65;          // DS:7c65
      std::array<char, 25>         visit_format;          // DS:7c66
      std::array<char, 10>         text_7c7f;             // DS:7c7f
      std::array<char, 6>          text_7c89;             // DS:7c89
      std::uint8_t                 unknown_7c8f;          // DS:7c8f
      std::uint16_t                hurricane_words[7];    // DS:7c90
      std::array<char, 6>          text_7c9e;             // DS:7c9e
      std::uint16_t                partisan_words[2];     // DS:7ca4: the partisan texts, in and out of the hurricane season
      std::array<std::uint8_t, 9>  unknown_7ca8;          // DS:7ca8
      std::array<char, 11>         text_7cb1;             // DS:7cb1
      std::uint16_t                resistance_names[2];   // DS:7cbc
      std::array<std::uint8_t, 29> angel_name;            // DS:7cc0
      std::array<char, 15>         angel_format;          // DS:7cdd
      std::array<FarPointer, 3>    angel_actions;         // DS:7cec: demoralize, inspire, judge by side (core/evangel)
      std::array<char, 15>         spy_name_format;       // DS:7cf8
      std::array<char, 19>         spy_laugh_text;        // DS:7d07
      std::array<char, 34>         spy_move_text;         // DS:7d1a
      std::array<char, 4>          spy_bold_off;          // DS:7d3c
      std::array<char, 10>         spy_humph_text;        // DS:7d40
      std::array<char, 11>         death_prefix;          // DS:7d4a
      std::array<char, 34>         death_suffix;          // DS:7d55
      std::array<char, 46>         comms_restored_format; // DS:7d77
      std::array<char, 47>         spy_captured_format;   // DS:7da5
      std::array<char, 38>         comms_unit_text;       // DS:7dd4
      std::array<char, 29>         spy_unit_text;         // DS:7dfa
      std::uint8_t                 unknown_7e17;          // DS:7e17
      std::uint16_t                quantity_divisors[8];  // DS:7e18
    };
  };
  std::array<char, 9>           request_supplies_name;     // DS:7e28
  std::array<char, 11>          request_battalions_name;   // DS:7e31
  std::array<char, 10>          request_regiments_name;    // DS:7e3c
  std::uint16_t                 request_kind_names[3];     // DS:7e46
  std::array<std::uint8_t, 3>   request_delay_bases;       // DS:7e4c
  std::array<char, 13>          equipment_delivery_format; // DS:7e4f
  std::array<std::uint8_t, 6>   request_weights2;          // DS:7e5c
  std::array<char, 23>          context_name_format;       // DS:7e62
  std::array<char, 146>         context_stats_format;      // DS:7e79
  std::array<char, 16>          context_blank_format;      // DS:7f0b
  std::uint8_t                  context_text_padding;      // DS:7f1b
  std::array<std::uint8_t, 6>   scenario_contexts;         // DS:7f1c
  std::array<char, 7>           menu_pass_label;           // DS:7f22
  std::array<char, 7>           menu_tact_label;           // DS:7f29
  std::array<char, 7>           menu_city_label;           // DS:7f30
  std::array<char, 6>           menu_tsfr_label;           // DS:7f37
  std::array<char, 7>           menu_ofcr_label;           // DS:7f3d
  std::array<char, 7>           menu_info_label;           // DS:7f44
  std::array<char, 7>           menu_orgz_label;           // DS:7f4b
  std::array<char, 7>           menu_rqst_label;           // DS:7f52
  std::array<char, 6>           menu_air_label;            // DS:7f59
  std::array<char, 7>           menu_spfc_label;           // DS:7f5f
  std::array<char, 7>           menu_game_label;           // DS:7f66
  std::array<char, 31>          control_prompt;            // DS:7f6d
  std::array<char, 39>          control_ready;             // DS:7f8c
  std::uint8_t                  control_text_padding;      // DS:7fb3
  std::array<FarPointer, 11>    control_actions;           // DS:7fb4: the routine of each command menu entry
  std::array<std::uint8_t, 144> control_menu;              // DS:7fe0
  union {
    std::array<std::uint8_t, 798> turn_text; // DS:8070..838e
    struct {
      std::array<char, 8>          direction_keys;          // DS:8070
      std::array<char, 48>         turn_label_format;       // DS:8078
      std::array<char, 6>          percent_text;            // DS:80a8
      std::array<char, 5>          not_available_text;      // DS:80ae
      std::array<char, 2>          text_80b3;               // DS:80b3
      std::uint8_t                 unknown_80b5;            // DS:80b5
      std::uint16_t                flash_state;             // DS:80b6
      std::array<char, 48>         regiment_position_text;  // DS:80b8
      std::array<char, 46>         division_position_text;  // DS:80e8
      std::array<std::uint8_t, 14> order_glyphs;            // DS:8116
      std::array<std::uint8_t, 18> view_clear_pattern;      // DS:8124
      std::uint16_t                panel_rows[3];           // DS:8136
      std::uint16_t                panel_columns[3];        // DS:813c
      std::array<char, 10>         hurricane_name;          // DS:8142
      std::array<char, 31>         turn_date_format;        // DS:814c
      std::array<char, 4>          turn_panel_text;         // DS:816b
      std::array<char, 4>          context_panel_text;      // DS:816f
      std::array<char, 4>          info_panel_text;         // DS:8173
      std::array<char, 4>          title_panel_text;        // DS:8177
      std::array<char, 4>          message_panel_text;      // DS:817b
      std::array<char, 9>          title_format;            // DS:817f
      std::array<char, 7>          speaker_text;            // DS:8188
      std::array<char, 12>         division_title;          // DS:818f
      std::array<char, 10>         commander_title;         // DS:819b
      std::array<char, 5>          speaker_name_format;     // DS:81a5
      std::array<char, 10>         speaker_body_text;       // DS:81aa
      std::array<char, 7>          speaker_end_text;        // DS:81b4
      std::array<char, 3>          menu_prompt_format;      // DS:81bb
      std::array<char, 2>          confirm_space;           // DS:81be
      std::array<char, 12>         confirm_ok_text;         // DS:81c0
      std::array<char, 43>         delay_hours_format;      // DS:81cc
      std::array<char, 27>         unit_report_format;      // DS:81f7
      std::array<char, 23>         objective_report_format; // DS:8212
      std::array<char, 31>         attack_format;           // DS:8229
      std::array<char, 14>         defense_format;          // DS:8248
      std::array<char, 15>         speed_format;            // DS:8256
      std::array<char, 15>         move_range_format;       // DS:8265
      std::array<char, 21>         gun_range_format;        // DS:8274
      std::array<char, 132>        group_element_format;    // DS:8289
      std::array<char, 49>         element_format;          // DS:830d
      std::array<char, 17>         cursor_position_format;  // DS:833e
      std::array<char, 9>          cursor_unit_format;      // DS:834f
      std::array<char, 8>          cursor_end;              // DS:8358
      std::array<char, 8>          pick_direction_keys;     // DS:8360
      std::array<std::uint8_t, 8>  general_wounded;         // DS:8368
      std::array<char, 7>          general_killed;          // DS:8370
      std::uint8_t                 unknown_8377;            // DS:8377
      std::array<char, 22>         general_casualty_format; // DS:8378
    };
  };
  std::array<std::uint8_t, 8> select_category_ranks; // DS:838e
  union {
    std::array<std::uint8_t, 3452> order_text; // DS:8396..9112
    struct {
      std::array<char, 17>         casualty_heading;        // DS:8396
      std::array<char, 18>         casualty_cut_selected;   // DS:83a7
      std::array<char, 35>         casualty_cut;            // DS:83b9
      std::array<char, 20>         casualty_loss_format;    // DS:83dc
      std::array<char, 22>         casualty_losses_format;  // DS:83f0
      std::array<char, 17>         casualty_wiped;          // DS:8406
      std::array<char, 22>         casualty_no_damage;      // DS:8417
      std::array<char, 44>         damage_supply_format;    // DS:842d
      std::array<char, 22>         damage_title;            // DS:8459
      std::array<char, 34>         damage_heading;          // DS:846f
      std::array<char, 14>         damage_element_format;   // DS:8491
      std::array<char, 22>         damage_none;             // DS:849f
      std::array<char, 20>         damage_loss_one;         // DS:84b5
      std::array<char, 22>         damage_loss_many;        // DS:84c9
      std::array<char, 42>         damage_endurance_format; // DS:84df
      std::uint8_t                 unknown_8509;            // DS:8509
      std::array<std::uint8_t, 8>  category_columns;        // DS:850a
      std::array<std::uint8_t, 6>  ai_categories;           // DS:8512
      std::array<char, 19>         target_text;             // DS:8518
      std::array<char, 7>          target_two_line_format;  // DS:852b: "%s\n%s?"
      std::array<char, 7>          target_one_line_format;  // DS:8532: "%s %s?"
      std::array<char, 17>         target_cell_format;      // DS:8539: "Move to (%d-%d)?"
      std::array<char, 31>         target_cancel_text;      // DS:854a
      std::array<char, 15>         cancel_orders_prompt;    // DS:8569: "Cancel orders?"
      std::array<char, 8>          text_8578;               // DS:8578
      std::array<char, 8>          text_8580;               // DS:8580
      std::array<char, 8>          text_8588;               // DS:8588
      std::array<char, 8>          text_8590;               // DS:8590
      std::array<char, 8>          text_8598;               // DS:8598
      std::array<char, 8>          text_85a0;               // DS:85a0
      std::array<char, 8>          text_85a8;               // DS:85a8
      std::array<char, 8>          text_85b0;               // DS:85b0
      std::array<char, 8>          text_85b8;               // DS:85b8
      std::array<char, 8>          text_85c0;               // DS:85c0
      std::array<char, 8>          text_85c8;               // DS:85c8
      std::array<char, 8>          text_85d0;               // DS:85d0
      std::array<char, 8>          text_85d8;               // DS:85d8
      std::array<char, 8>          text_85e0;               // DS:85e0
      std::array<char, 8>          text_85e8;               // DS:85e8
      std::array<char, 8>          text_85f0;               // DS:85f0
      std::array<char, 8>          text_85f8;               // DS:85f8
      std::array<char, 8>          text_8600;               // DS:8600
      std::array<char, 8>          text_8608;               // DS:8608
      std::array<char, 8>          text_8610;               // DS:8610
      std::array<char, 8>          text_8618;               // DS:8618
      std::array<char, 8>          text_8620;               // DS:8620
      std::array<char, 8>          text_8628;               // DS:8628
      std::array<char, 8>          text_8630;               // DS:8630
      std::array<char, 9>          text_8638;               // DS:8638
      std::array<char, 9>          text_8641;               // DS:8641
      std::array<char, 13>         text_864a;               // DS:864a
      std::array<char, 13>         text_8657;               // DS:8657
      std::array<std::array<MenuItem, 9>, 4> order_menus; // DS:8664: the Orders command's menus, eight orders each
      std::array<MenuItem, 3>      supply_choices; // DS:8814: supply a city or a regiment
      std::array<MenuItem, 3>      repair_choices; // DS:8838: repair a city or a regiment
      std::array<ScreenPoint, 8>   order_spots;    // DS:885c: where the eight orders of a menu go
      std::array<ScreenPoint, 2>   choice_spots;   // DS:887c: where the two choices go
      OrderPrompts                 issue_prompts; // DS:8884: the Orders command's messages
      std::array<std::array<std::uint8_t, 8>, 4> issue_order_codes; // DS:89bc: the orders of each menu, 0xff-padded
      std::array<char, 27>         awaiting_orders; // DS:89dc: "Awaiting your orders, sir."
      OrderPrompts                 action_prompts;  // DS:89f7: the map cursor's unit action messages
      std::uint8_t                 unknown_8b2f;
      std::array<std::array<std::uint8_t, 8>, 3> action_order_codes; // DS:8b30: as issue_order_codes
      std::array<char, 24>         select_division; // DS:8b48
      std::array<char, 21>         select_order;            // DS:8b60
      std::array<char, 43>         order_whole_division;    // DS:8b75
      std::array<char, 24>         select_regiment;         // DS:8ba0
      std::array<char, 39>         stock_header;              // DS:8bb8
      std::array<char, 31>         stock_fuel_line;               // DS:8bdf
      std::array<char, 31>         stock_ammo_line;               // DS:8bfe
      std::array<char, 37>         stock_food_line;               // DS:8c1d
      std::array<char, 33>         stock_question_short;               // DS:8c42
      std::array<char, 55>         stock_question;               // DS:8c63
      std::array<char, 5>          stock_fuel;               // DS:8c9a
      std::array<char, 5>          stock_ammo;               // DS:8c9f
      std::array<char, 5>          stock_food;               // DS:8ca4
      std::array<char, 22>         stock_no_cities;               // DS:8ca9
      std::array<char, 15>         stock_select_city;               // DS:8cbf
      std::array<char, 36>         transfer_which_division; // DS:8cce
      std::array<char, 16>         transfer_which_regiment; // DS:8cf2
      std::array<char, 13>         transfer_which_group;    // DS:8d02
      std::array<char, 5>          transfer_hour;           // DS:8d0f
      std::array<char, 6>          transfer_hours_text;     // DS:8d14
      std::array<char, 50>         transfer_confirm;        // DS:8d1a: "It will take about %u %s ..."
      std::array<char, 29>         rest_thanks_text; // DS:8d4c: "Thank you very much, sir!"
      std::array<char, 26>         assign_thanks_text; // DS:8d69: "You can count on me, sir!"
      std::array<char, 55>         division_charge_prompt; // DS:8d83: "Which division commander do you want to put in charge?"
      std::array<std::uint8_t, 6>  post_label_none; // DS:8dba: the empty label of the posts menu
      std::array<char, 8>          text_8dc0;               // DS:8dc0
      std::array<char, 8>          text_8dc8;               // DS:8dc8
      std::array<char, 8>          text_8dd0;               // DS:8dd0
      std::array<char, 8>          text_8dd8;               // DS:8dd8
      std::array<char, 10>         text_8de0;               // DS:8de0
      std::array<char, 8>          text_8dea;               // DS:8dea
      std::array<char, 28>         none_available_text; // DS:8df2: "No one is available, sir."
      std::array<char, 21>         assign_who_prompt; // DS:8e0e: "Who will you assign?"
      std::uint8_t                 no_holder_1; // DS:8e23: "": a division without a commander
      std::array<char, 7>          post_format_1; // DS:8e24: "%s%-8s": post label, holder
      std::uint8_t                 no_holder_2; // DS:8e2b: ""
      std::array<char, 7>          post_format_2; // DS:8e2c: "%s%-8s"
      std::uint8_t                 no_holder_3; // DS:8e33: ""
      std::array<char, 7>          post_format_3; // DS:8e34: "%s%-8s"
      std::uint8_t                 no_holder_4; // DS:8e3b: ""
      std::array<char, 7>          post_format_4; // DS:8e3c: "%s%-8s"
      std::array<char, 7>          post_format_5; // DS:8e43: "%s%-8s"
      std::array<char, 7>          post_format_6; // DS:8e4a: "%s%-8s"
      std::array<char, 33>         assign_where_prompt; // DS:8e51: "Where should %s be assigned, sir"
      std::array<MenuItem, 7>      assign_menu; // DS:8e72: the posts menu, a button per post (labels filled in)
      std::uint16_t                post_labels[6]; // DS:8ec6: "Div 1: " .. "Div 4: ", "Ops Ofc: ", "Intel: "
      std::array<std::uint8_t, 10> inactive_division_label; // DS:8ed2: " ------- ": a division not in the field
      std::array<char, 41>         none_to_rest_text; // DS:8edc: "There are no commanders who can rest."
      std::array<char, 19>         rest_who_prompt; // DS:8f05: "Who shall we rest?"
      std::array<char, 45>         rest_days_prompt; // DS:8f18: "How many days should Commander %s take leave"
      std::array<char, 9>          text_8f45;               // DS:8f45
      std::array<char, 8>          text_8f4e;               // DS:8f4e
      std::array<char, 5>          text_8f56;               // DS:8f56
      std::array<char, 16>         personnel_prompt; // DS:8f5b: "Select an order"
      std::uint8_t                 unknown_8f6b;            // DS:8f6b
      std::array<FarPointer, 3>    personnel_actions; // DS:8f6c: reassign, appoint, rest
      std::array<MenuItem, 4>      personnel_menu; // DS:8f78: "Reassign", "Appoint", "Rest"
      std::array<FarPointer, 20>   advice_tips; // DS:8fa8: the twenty general tips, far pointers to advice texts
      std::array<char, 16>         text_8ff8;               // DS:8ff8
      std::array<char, 16>         text_9008;               // DS:9008
      std::array<char, 17>         text_9018;               // DS:9018
      std::array<char, 12>         text_9029;               // DS:9029
      std::array<char, 16>         text_9035;               // DS:9035
      std::array<char, 27>         advice_prompt;           // DS:9045
      std::array<FarPointer, 5>    advice_actions;          // DS:9060: the action of each advice topic
      std::uint16_t                advice_menu[4];          // DS:9074
      std::uint16_t                near_907c;               // DS:907c
      std::array<std::uint8_t, 10> bytes_907e;              // DS:907e
      std::uint16_t                near_9088;               // DS:9088
      std::array<std::uint8_t, 10> bytes_908a;              // DS:908a
      std::uint16_t                near_9094;               // DS:9094
      std::array<std::uint8_t, 10> bytes_9096;              // DS:9096
      std::uint16_t                near_90a0;               // DS:90a0
      std::array<std::uint8_t, 10> bytes_90a2;              // DS:90a2
      std::uint16_t                near_90ac;               // DS:90ac
      std::array<std::uint8_t, 14> bytes_90ae;              // DS:90ae
      std::array<char, 11>         text_90bc;               // DS:90bc
      std::array<char, 13>         text_90c7;               // DS:90c7
      std::array<char, 14>         staff_prompt;            // DS:90d4
      std::array<FarPointer, 3>    staff_actions;           // DS:90e2: the action of each staff officer
      std::uint16_t                staff_menu[4];           // DS:90ee
      std::uint16_t                near_90f6;               // DS:90f6
      std::array<std::uint8_t, 10> bytes_90f8;              // DS:90f8
      std::uint16_t                near_9102;               // DS:9102
      std::array<std::uint8_t, 14> bytes_9104;              // DS:9104
    };
  };
  union {
    std::array<std::uint8_t, 3326> supply_text; // DS:9112..9e10
    struct {
      std::array<char, 53>         panel_fuel_ammo;   // DS:9112
      std::array<char, 50>         panel_food_end;    // DS:9147
      std::array<char, 33>         panel_garrison;    // DS:9179
      std::array<char, 28>         panel_stock_row;   // DS:919a
      std::array<char, 13>         panel_no_stock;    // DS:91b6
      std::uint8_t                 unknown_91c3;      // DS:91c3
      std::array<ScreenPoint, 6> stock_rows; // DS:91c4: where the city panel prints its stocks
      std::array<char, 8>          panel_waiting;     // DS:91dc
      std::array<char, 53>         panel_orders;      // DS:91e4
      std::array<char, 51>         panel_target;      // DS:9219
      std::array<char, 53>         panel_fatigue;     // DS:924c
      std::array<char, 28>         panel_element_row; // DS:9281
      std::uint8_t                 unknown_929d;      // DS:929d
      std::array<ScreenPoint, 6> element_rows; // DS:929e: where the regiment panel prints its battalions
      std::array<char, 13>         text_92b6;         // DS:92b6
      std::array<char, 12>         text_92c3;         // DS:92c3
      std::array<char, 12>         text_92cf;         // DS:92cf
      std::array<char, 12>         text_92db;         // DS:92db
      std::array<char, 48>         contact_prompt;    // DS:92e7
      std::array<char, 44>         contact_lost;      // DS:9317
      std::uint8_t                 unknown_9343;      // DS:9343
      std::uint16_t                contact_menu[4];   // DS:9344
      std::uint16_t                near_934c;         // DS:934c
      std::array<std::uint8_t, 10> bytes_934e;        // DS:934e
      std::uint16_t                near_9358;         // DS:9358
      std::array<std::uint8_t, 10> bytes_935a;        // DS:935a
      std::uint16_t                near_9364;         // DS:9364
      std::array<std::uint8_t, 10> bytes_9366;        // DS:9366
      std::uint16_t                near_9370;         // DS:9370
      std::array<std::uint8_t, 14> bytes_9372;        // DS:9372
      std::array<char, 17>         people_here;       // DS:9380
      std::array<char, 23>         people_none;       // DS:9391
      std::array<char, 9>          text_93a8;         // DS:93a8
      std::array<char, 6>          text_93b1;         // DS:93b1
      std::array<std::uint8_t, 31> people_prompt;     // DS:93b7
      std::array<FarPointer, 2> people_actions; // DS:93d6: the menu's actions
      std::uint16_t                people_menu[4];    // DS:93de
      std::uint16_t                near_93e6;         // DS:93e6
      std::array<std::uint8_t, 10> bytes_93e8;        // DS:93e8
      std::uint16_t                near_93f2;         // DS:93f2
      std::array<std::uint8_t, 14> bytes_93f4;        // DS:93f4
      std::array<char, 26>         city_prompt;       // DS:9402
      std::array<char, 3>          city_format;       // DS:941c
      std::array<char, 31>         city_none_own;     // DS:941f
      std::array<char, 30>         city_none_enemy;   // DS:943e
      std::array<char, 9>          text_945c;         // DS:945c
      std::array<char, 6>          text_9465;         // DS:9465
      union {                                         // DS:946b
        std::uint16_t                city_side_prompt[15];
        std::array<std::uint8_t, 30> city_side_prompt_bytes;
      };
      std::uint8_t                 unknown_9489;         // DS:9489
      std::array<FarPointer, 2> city_actions; // DS:948a: the menu's actions
      std::uint16_t                city_side_menu[4];    // DS:9492
      std::uint16_t                near_949a;            // DS:949a
      std::array<std::uint8_t, 10> bytes_949c;           // DS:949c
      std::uint16_t                near_94a6;            // DS:94a6
      std::array<std::uint8_t, 14> bytes_94a8;           // DS:94a8
      std::array<char, 35>         regiment_prompt;      // DS:94b6
      std::array<char, 3>          regiment_format;      // DS:94d9
      std::array<char, 30>         view_division_prompt; // DS:94dc
      std::array<char, 27>         units_none_enemy;     // DS:94fa
      std::array<char, 9>          text_9515;            // DS:9515
      std::array<char, 6>          text_951e;            // DS:951e
      std::uint16_t                info_prompt[15];      // DS:9524
      std::array<FarPointer, 2> info_actions; // DS:9542: the menu's actions
      std::uint16_t                info_menu[4];         // DS:954a
      std::uint16_t                near_9552;            // DS:9552
      std::array<std::uint8_t, 10> bytes_9554;           // DS:9554
      std::uint16_t                near_955e;            // DS:955e
      std::array<std::uint8_t, 14> bytes_9560;           // DS:9560
      std::array<char, 29>         weapons_prompt;       // DS:956e
      std::array<char, 19>         weapons_here;         // DS:958b
      std::array<char, 9>          text_959e;            // DS:959e
      std::array<char, 6>          text_95a7;            // DS:95a7
      union {                                            // DS:95ad
        std::uint16_t                arms_prompt[10];
        std::array<std::uint8_t, 20> arms_prompt_bytes;
      };
      std::uint8_t                 unknown_95c1;   // DS:95c1
      std::array<FarPointer, 2> arms_actions; // DS:95c2: the menu's actions
      std::uint16_t                arms_menu[4];   // DS:95ca
      std::uint16_t                near_95d2;      // DS:95d2
      std::array<std::uint8_t, 10> bytes_95d4;     // DS:95d4
      std::uint16_t                near_95de;      // DS:95de
      std::array<std::uint8_t, 14> bytes_95e0;     // DS:95e0
      std::array<char, 19>         victory_title;  // DS:95ee
      std::array<char, 15>         victory_format; // DS:9601
      std::array<char, 28>         victory_orders; // DS:9610
      std::array<char, 10>         text_962c;      // DS:962c
      std::array<char, 9>          text_9636;      // DS:9636
      std::array<char, 7>          text_963f;      // DS:963f
      std::array<char, 6>          text_9646;      // DS:9646
      std::array<char, 10>         text_964c;      // DS:964c
      std::array<char, 19>         text_9656;      // DS:9656
      union {                                      // DS:9669
        std::uint16_t                view_main_prompt[18];
        std::array<std::uint8_t, 36> view_main_prompt_bytes;
      };
      std::uint8_t                 unknown_968d;            // DS:968d
      std::array<FarPointer, 6> view_main_actions; // DS:968e: the menu's actions
      std::uint16_t                view_main_items[4];      // DS:96a6
      std::uint16_t                near_96ae;               // DS:96ae
      std::array<std::uint8_t, 10> bytes_96b0;              // DS:96b0
      std::uint16_t                near_96ba;               // DS:96ba
      std::array<std::uint8_t, 10> bytes_96bc;              // DS:96bc
      std::uint16_t                near_96c6;               // DS:96c6
      std::array<std::uint8_t, 10> bytes_96c8;              // DS:96c8
      std::uint16_t                near_96d2;               // DS:96d2
      std::array<std::uint8_t, 10> bytes_96d4;              // DS:96d4
      std::uint16_t                near_96de;               // DS:96de
      std::array<std::uint8_t, 10> bytes_96e0;              // DS:96e0
      std::uint16_t                near_96ea;               // DS:96ea
      std::array<std::uint8_t, 14> bytes_96ec;              // DS:96ec
      std::array<char, 29>         ops_chief_format;        // DS:96fa
      std::array<char, 9>          number_range_format;     // DS:9717
      std::uint16_t                supply_unlimited;        // DS:9720: 60000, the capacity of a city stockpile
      std::uint16_t                supply_panel_x;          // DS:9722
      std::uint16_t                supply_panel_y;          // DS:9724
      std::array<char, 15>         supply_name_format;      // DS:9726
      std::array<char, 12>         supply_name_text;        // DS:9735
      std::array<char, 3>          text_9741;               // DS:9741
      std::array<char, 3>          text_9744;               // DS:9744
      std::array<char, 3>          text_9747;               // DS:9747
      std::array<char, 10>         supply_row_left;         // DS:974a
      std::array<char, 4>          supply_row_right;        // DS:9754
      std::array<char, 7>          supply_row_end;          // DS:9758
      std::uint8_t                 unknown_975f;            // DS:975f
      std::uint16_t                near_9760;               // DS:9760
      std::uint16_t                near_9762;               // DS:9762
      std::uint16_t                near_9764;               // DS:9764
      std::array<std::uint8_t, 24> supply_prompt;           // DS:9766
      std::array<char, 10>         supply_continue;         // DS:977e
      std::array<char, 6>          tank_suffix;             // DS:9788
      std::array<std::array<char, 4>, 7> nation_codes; // DS:978e: GER ITY USA ENG FRN RUS, and USA for 12
      std::array<char, 5> nation_code_other; // DS:97aa: the blank code of any other nation value
      std::array<char, 4>          text_97af;               // DS:97af
      std::uint8_t                 unknown_97b3;            // DS:97b3
      ListWindow                   supply_type_list;        // DS:97b4: the allocation screen's list of types
      std::array<char, 30>         supply_header_a;         // DS:97c7
      std::array<char, 32>         supply_row_a;            // DS:97e5
      std::array<char, 4>          text_9805;               // DS:9805
      std::array<char, 40>         supply_empty_a;          // DS:9809
      std::uint8_t                 unknown_9831;            // DS:9831
      ListWindow                   supply_slot_list;        // DS:9832: the allocation screen's list of the units' slots
      std::array<char, 32>         supply_header_b;         // DS:9845
      std::array<char, 23>         supply_row_b;            // DS:9865
      std::array<char, 30>         supply_empty_b;          // DS:987c
      std::array<char, 17>         alloc_amount_format;     // DS:989a
      std::array<char, 15>         alloc_first_format;      // DS:98ab
      std::array<char, 15>         alloc_second_format;     // DS:98ba
      std::array<char, 29>         alloc_heading;           // DS:98c9
      std::array<char, 22>         alloc_row_format;        // DS:98e6
      std::array<char, 15>         alloc_name_format;       // DS:98fc
      std::array<char, 124>        alloc_stats_format;      // DS:990b
      std::array<char, 31>         regiment_hq_prompt;      // DS:9987
      std::array<char, 32>         organize_none;           // DS:99a6
      std::array<char, 25>         organize_which;          // DS:99c6
      std::array<char, 26>         organize_confirm_format; // DS:99df
      std::array<char, 20>         organize_no_arms;        // DS:99f9
      std::array<char, 49>         organize_no_neighbor;    // DS:9a0d
      std::array<char, 30>         organize_with;           // DS:9a3e
      std::array<char, 40>         organize_full;           // DS:9a5c
      std::array<char, 16>         organize_dispatch;       // DS:9a84
      std::array<char, 70>         organize_complete;       // DS:9a94
      std::array<char, 20>         organize_cancel;         // DS:9ada
      std::array<char, 20>         organize_select;         // DS:9aee
      std::array<char, 24>         organize_number;         // DS:9b02
      std::array<char, 40>         rq_no_more_text;         // DS:9b1a
      std::array<char, 51>         rq_cancel_text;          // DS:9b42
      std::array<char, 37>         rq_filed_text;           // DS:9b75
      std::array<char, 9>          rq_supplies_text;        // DS:9b9a
      std::array<char, 17>         rq_supplies_title;       // DS:9ba3
      std::array<char, 51>         rq_supplies_where;       // DS:9bb4
      std::array<char, 26>         rq_supplies_confirm;     // DS:9be7
      std::array<char, 8>          text_9c01;               // DS:9c01
      std::uint8_t                 unknown_9c09;            // DS:9c09
      ListWindow                   rq_type_dialog;          // DS:9c0a: the battalion types of the arms request
      std::array<char, 38>         rq_type_header;          // DS:9c1d
      std::array<char, 9>          rq_type_name;            // DS:9c43
      std::array<char, 35>         rq_type_row;             // DS:9c4c
      std::array<char, 5>          rq_arms_text;            // DS:9c6f
      std::array<char, 44>         rq_arms_which;           // DS:9c74
      std::array<char, 50>         rq_arms_where;           // DS:9ca0
      std::array<char, 26>         rq_arms_confirm;         // DS:9cd2
      std::array<char, 6>          text_9cec;               // DS:9cec
      ListWindow                   rq_regiment_list;        // DS:9cf2: the free regiments of the regiment request
      std::array<char, 24>         rq_unit_header;          // DS:9d05
      std::array<char, 9>          rq_unit_name;            // DS:9d1d
      std::array<char, 25>         rq_unit_row;             // DS:9d26
      std::array<char, 9>          text_9d3f;               // DS:9d3f
      std::array<char, 11>         text_9d48;               // DS:9d48
      std::array<char, 8>          text_9d53;               // DS:9d53
      std::array<char, 10>         text_9d5b;               // DS:9d5b
      std::array<char, 15>         rq_reinforcements_text;  // DS:9d65
      std::array<char, 9>          rq_regiment_text;        // DS:9d74
      std::array<char, 10>         rq_battalion_text;       // DS:9d7d
      std::array<char, 39>         rq_which_regiment_text;  // DS:9d87
      std::array<char, 10>         rq_regiments_text;       // DS:9dae
      std::array<char, 27>         rq_regiment_kind_text;   // DS:9db8
      std::uint8_t                 unknown_9dd3;            // DS:9dd3
      std::uint16_t                rq_slash_text[4];        // DS:9dd4
      std::uint16_t                near_9ddc;               // DS:9ddc
      std::array<std::uint8_t, 10> bytes_9dde;              // DS:9dde
      std::uint16_t                near_9de8;               // DS:9de8
      std::array<std::uint8_t, 10> bytes_9dea;              // DS:9dea
      std::uint16_t                near_9df4;               // DS:9df4
      std::array<std::uint8_t, 10> bytes_9df6;              // DS:9df6
      std::uint16_t                near_9e00;               // DS:9e00
      std::array<std::uint8_t, 14> bytes_9e02;              // DS:9e02
    };
  };
  union {
    std::array<std::uint8_t, 4126> air_text; // DS:9e10..ae2e
    struct {
      std::array<char, 43>         rq_header_text;          // DS:9e10
      std::array<char, 9>          text_9e3b;               // DS:9e3b
      std::array<char, 11>         text_9e44;               // DS:9e44
      std::array<char, 6>          text_9e4f;               // DS:9e4f
      std::array<char, 10>         text_9e55;               // DS:9e55
      std::array<char, 9>          text_9e5f;               // DS:9e5f
      std::array<char, 7>          text_9e68;               // DS:9e68
      std::array<char, 9>          text_9e6f;               // DS:9e6f
      std::array<char, 11>         text_9e78;               // DS:9e78
      std::array<char, 10>         text_9e83;               // DS:9e83
      std::array<char, 8>          rq_title_text;           // DS:9e8d
      std::array<char, 44>         rq_supply_row_text;      // DS:9e95
      std::array<char, 44>         rq_supply_filed_text;    // DS:9ec1
      std::array<char, 43>         rq_battalion_row_text;   // DS:9eed
      std::array<char, 42>         rq_battalion_filed_text; // DS:9f18
      std::array<char, 47>         rq_regiment_row_text;    // DS:9f42
      std::array<char, 46>         rq_regiment_filed_text;  // DS:9f71
      std::array<char, 18>         rq_menu_prompt_text;     // DS:9f9f
      std::array<char, 63>         rq_busy_text;            // DS:9fb1
      std::array<char, 31>         rq_no_city_text;         // DS:9ff0
      std::uint8_t                 unknown_a00f;            // DS:a00f
      std::uint16_t                rq_regiment_kind_names[6]; // DS:a010: near pointers to the regiment kinds' names
      std::array<std::uint8_t, 2>  bytes_a01c;              // DS:a01c
      std::array<FarPointer, 3>    request_handlers; // DS:a01e: the request menu's supplies, battalion and regiment handlers
      std::uint16_t                rq_menu_items[4];        // DS:a02a
      std::uint16_t                near_a032;               // DS:a032
      std::array<std::uint8_t, 10> bytes_a034;              // DS:a034
      std::uint16_t                near_a03e;               // DS:a03e
      std::array<std::uint8_t, 10> bytes_a040;              // DS:a040
      std::uint16_t                near_a04a;               // DS:a04a
      std::array<std::uint8_t, 14> bytes_a04c;              // DS:a04c
      std::array<char, 3> air_question_mark; // DS:a05a: the number prompt's "? " in the mouse input mode
      std::array<char, 13> air_range_format; // DS:a05d: "(%d-%d)? "
      std::array<char, 4> air_input_begin; // DS:a06a
      std::array<char, 4> air_input_end; // DS:a06e
      std::array<char, 13> air_hostile_city; // DS:a072: "hostile city"
      std::array<char, 25>         air_understood;               // DS:a07f
      std::array<char, 17>         text_a098;               // DS:a098
      std::array<char, 17>         text_a0a9;               // DS:a0a9
      std::array<char, 11>         text_a0ba;               // DS:a0ba
      std::array<char, 6>          air_plane;          // DS:a0c5
      std::array<char, 7>          air_planes;         // DS:a0cb
      std::array<char, 5>          air_hour;           // DS:a0d2
      std::array<char, 6>          air_hours;          // DS:a0d7
      std::uint8_t                 unknown_a0dd;            // DS:a0dd
      std::array<BombingFlight, 3> bombing_flights; // DS:a0de: the planes each bombing mission type sends
      std::uint16_t                bombing_names[3]; // DS:a0e4: near pointers to the mission type names
      std::array<std::uint8_t, 30> bytes_a0ea;              // DS:a0ea
      std::array<char, 29>         text_a108;               // DS:a108
      std::array<char, 29>         text_a125;               // DS:a125
      std::array<char, 10>         air_null_order;               // DS:a142
      std::array<char, 15>         bombing_title;               // DS:a14c
      std::array<char, 22>         bombing_site_prompt;               // DS:a15b
      std::uint8_t                 unknown_a171;            // DS:a171
      std::array<std::uint8_t, 20> bombing_menu; // DS:a172: the mission type menu
      std::uint16_t                near_a186;               // DS:a186
      std::uint16_t                words_a188[5];           // DS:a188
      std::uint16_t                near_a192;               // DS:a192
      std::array<std::uint8_t, 14> bytes_a194;              // DS:a194
      std::array<char, 37>         drop_ask_troops;               // DS:a1a2
      std::array<char, 26>         drop_troops_assigned;               // DS:a1c7
      std::array<char, 39>         drop_which_regiment;               // DS:a1e1
      std::array<char, 26>         drop_assign_soldiers;               // DS:a208
      std::array<char, 45>         drop_command_post;               // DS:a222
      std::array<char, 25>         drop_understood;               // DS:a24f
      std::array<char, 6>          drop_plane;          // DS:a268
      std::array<char, 7>          drop_planes;         // DS:a26e
      std::array<char, 34>         drop_no_paratroopers;               // DS:a275
      std::array<char, 16>         drop_cancel;               // DS:a297
      std::array<char, 46>         drop_no_transports;               // DS:a2a7
      std::array<char, 41>         drop_no_regiments;               // DS:a2d5
      std::array<char, 42>         drop_no_battalions;               // DS:a2fe
      std::array<char, 45>         drop_where;               // DS:a328
      std::array<char, 13>         drop_paratroopers;               // DS:a355
      std::array<char, 33>         drop_hours_prompt;               // DS:a362
      std::array<char, 71>         drop_ask_hq;               // DS:a383
      std::array<char, 44> lift_lead_format; // DS:a3ca: the officer needs at least %d hours
      std::array<char, 25> lift_understood; // DS:a3f6
      std::array<char, 6>          air_lift_plane;          // DS:a40f
      std::array<char, 7>          air_lift_planes;         // DS:a415
      std::array<char, 5>          air_lift_hour;           // DS:a41c
      std::array<char, 6>          air_lift_hours;          // DS:a421
      std::array<char, 47> lift_start_format; // DS:a427: In %d %s, %d %s will start their cargo lift
      std::array<char, 20> lift_cancel; // DS:a456: cancel the pending airlift?
      std::array<char, 47> lift_no_planes; // DS:a46a
      std::array<char, 22> lift_ask_ammo; // DS:a499
      std::array<char, 22> lift_ask_fuel; // DS:a4af
      std::array<char, 22> lift_ask_food; // DS:a4c5
      std::array<char, 25> lift_select_point; // DS:a4db
      std::array<char, 6> lift_cargo_name; // DS:a4f4: "cargo"
      std::array<char, 23> lift_ask_hours; // DS:a4fa
      std::array<char, 65> lift_confirm; // DS:a511
      std::array<char, 29> air_header_format; // DS:a552: the mission table header
      std::array<char, 11>         text_a56f;               // DS:a56f
      std::array<char, 13>         text_a57a;               // DS:a57a
      std::array<char, 11>         text_a587;               // DS:a587
      std::array<char, 10> air_title; // DS:a592
      std::array<char, 42> air_strike_format; // DS:a59c
      std::array<char, 42> air_drop_format; // DS:a5c6
      std::array<char, 42> air_lift_format; // DS:a5f0
      std::array<char, 23> air_mission_question; // DS:a61a: which mission to plan
      std::uint8_t                 unknown_a631;            // DS:a631
      std::array<FarPointer, 3>    air_missions; // DS:a632: the routines of the strike, drop and airlift missions
      std::uint16_t                air_mission_menu[4]; // DS:a63e: the mission menu
      std::uint16_t                near_a646;               // DS:a646
      std::array<std::uint8_t, 10> bytes_a648;              // DS:a648
      std::uint16_t                near_a652;               // DS:a652
      std::array<std::uint8_t, 10> bytes_a654;              // DS:a654
      std::uint16_t                near_a65e;               // DS:a65e
      std::array<std::uint8_t, 14> bytes_a660;              // DS:a660
      std::array<char, 5>          raid_title;               // DS:a66e
      std::array<char, 19>         raid_prompt;               // DS:a673
      std::array<char, 17>         raid_city_question;               // DS:a686
      std::array<char, 9>          raid_unit_question;               // DS:a697
      std::array<char, 39>         raid_refusal_1;               // DS:a6a0
      std::array<char, 53>         raid_refusal_2;               // DS:a6c7
      std::array<char, 51>         raid_refusal_3;               // DS:a6fc
      std::array<char, 36>         raid_refusal_4;               // DS:a72f
      std::array<char, 35>         raid_lieutenant;               // DS:a753
      std::array<char, 9>          sabotage_title;               // DS:a776
      std::array<char, 38>         sabotage_prompt;               // DS:a77f
      std::array<char, 15>         sabotage_city_question;               // DS:a7a5
      std::array<char, 42>         sabotage_refusal_1;               // DS:a7b4
      std::array<char, 47>         sabotage_refusal_2;               // DS:a7de
      std::array<char, 49>         sabotage_refusal_3;               // DS:a80d
      std::array<char, 34>         sabotage_refusal_4;               // DS:a83e
      std::array<char, 60>         sabotage_duration;               // DS:a860
      std::array<char, 40>         sabotage_lieutenant;               // DS:a89c
      std::array<char, 9>          cut_comm_title;               // DS:a8c4
      std::array<char, 34>         cut_comm_prompt;               // DS:a8cd
      std::array<char, 28>         cut_comm_city_question;               // DS:a8ef
      std::array<char, 9>          cut_comm_unit_question;               // DS:a90b
      std::array<char, 35>         cut_comm_refusal_1;               // DS:a914
      std::array<char, 44>         cut_comm_refusal_2;               // DS:a937
      std::array<char, 53>         cut_comm_refusal_3;               // DS:a963
      std::array<char, 53>         cut_comm_refusal_4;               // DS:a998
      std::array<char, 5>          cut_comm_hour;       // DS:a9cd
      std::array<char, 6>          cut_comm_hours;      // DS:a9d2
      std::array<char, 48>         cut_comm_duration;               // DS:a9d8
      std::array<char, 45>         cut_comm_lieutenant;               // DS:aa08
      std::array<char, 11>         assassinate_title;               // DS:aa35
      std::array<char, 28>         assassinate_prompt;               // DS:aa40
      std::array<char, 27>         assassinate_unit_question;               // DS:aa5c
      std::array<char, 22>         assassinate_refusal_1;               // DS:aa77
      std::array<char, 47>         assassinate_refusal_2;               // DS:aa8d
      std::array<char, 19>         assassinate_refusal_3;               // DS:aabc
      std::array<char, 47>         assassinate_refusal_4;               // DS:aacf
      std::array<char, 58>         assassinate_duration;               // DS:aafe
      std::array<char, 35>         assassinate_lieutenant;               // DS:ab38
      std::array<char, 6>          recon_title;               // DS:ab5b
      std::array<char, 23>         recon_prompt;               // DS:ab61
      std::array<char, 9>          recon_city_question;               // DS:ab78
      std::array<char, 15>         recon_unit_question;               // DS:ab81
      std::array<char, 38>         recon_refusal_1;               // DS:ab90
      std::array<char, 48>         recon_refusal_2;               // DS:abb6
      std::array<char, 39>         recon_refusal_3;               // DS:abe6
      std::array<char, 5>          recon_hour;       // DS:ac0d
      std::array<char, 6>          recon_hours;      // DS:ac12
      std::array<char, 43>         recon_duration;               // DS:ac18
      std::array<char, 37>         recon_lieutenant;               // DS:ac43
      std::array<char, 6>          text_ac68;               // DS:ac68
      std::array<char, 10>         text_ac6e;               // DS:ac6e
      std::array<char, 9>          text_ac78;               // DS:ac78
      std::array<char, 5>          text_ac81;               // DS:ac81
      std::array<char, 12>         text_ac86;               // DS:ac86
      std::array<char, 10>         text_ac92;               // DS:ac92
      std::array<char, 10>         text_ac9c;               // DS:ac9c
      std::array<char, 12>         text_aca6;               // DS:aca6
      std::array<char, 14>         text_acb2;               // DS:acb2
      std::array<char, 14>         text_acc0;               // DS:acc0
      std::array<char, 15>         text_acce;               // DS:acce
      std::array<char, 11>         text_acdd;               // DS:acdd
      std::array<char, 11>         text_ace8;               // DS:ace8
      std::array<char, 15>         special_forces_title;               // DS:acf3
      std::array<char, 26>         special_orders_line;               // DS:ad02
      std::array<char, 26>         special_target_line;               // DS:ad1c
      std::array<char, 26>         special_time_line;               // DS:ad36
      std::array<char, 74>         special_cancel_question;               // DS:ad50
      std::array<char, 27>         special_select_question;               // DS:ad9a
      std::uint8_t                 unknown_adb5;            // DS:adb5
      std::array<FarPointer, 5>    special_missions;        // DS:adb6: raid, sabotage, cut comm, assassinate, recon
      std::uint16_t                special_menu[4];           // DS:adca
      std::uint16_t                near_add2;               // DS:add2
      std::array<std::uint8_t, 10> bytes_add4;              // DS:add4
      std::uint16_t                near_adde;               // DS:adde
      std::array<std::uint8_t, 10> bytes_ade0;              // DS:ade0
      std::uint16_t                near_adea;               // DS:adea
      std::array<std::uint8_t, 10> bytes_adec;              // DS:adec
      std::uint16_t                near_adf6;               // DS:adf6
      std::array<std::uint8_t, 10> bytes_adf8;              // DS:adf8
      std::uint16_t                near_ae02;               // DS:ae02
      std::array<std::uint8_t, 14> bytes_ae04;              // DS:ae04
      std::uint16_t                special_mission_names[8]; // DS:ae12: near pointers to the names, by mission code
      std::array<std::uint8_t, 12> unknown_ae22;            // DS:ae22
    };
  };
  union {
    std::array<std::uint8_t, 1044> settings_data; // DS:ae2e..b242
    struct {
      std::array<char, 7> settings_aerial; // DS:ae2e
      std::array<char, 6> settings_field; // DS:ae35
      std::array<char, 3> settings_pictures_on; // DS:ae3b
      std::array<char, 4> settings_pictures_off; // DS:ae3e
      std::array<char, 5> settings_battles_view; // DS:ae42
      std::array<char, 3> settings_battles_no; // DS:ae47
      std::array<char, 7> settings_battles_select; // DS:ae4a
      std::array<char, 3> settings_music_on; // DS:ae51
      std::array<char, 4> settings_music_off; // DS:ae54
      std::array<char, 115> settings_summary; // DS:ae58
      std::array<char, 5> battles_label_view; // DS:aecb
      std::array<char, 8> battles_label_no_view; // DS:aed0
      std::array<char, 7> battles_label_select; // DS:aed8
      std::array<char, 17> battles_view_all; // DS:aedf
      std::array<char, 16> battles_view_none; // DS:aef0
      std::array<char, 23> battles_view_selected; // DS:af00
      std::array<char, 27> battles_question; // DS:af17
      std::array<char, 13> battles_reply; // DS:af32
      std::uint8_t                  settings_padding;         // DS:af3f
      std::array<std::uint8_t, 48> battles_menu; // DS:af40
      std::uint16_t battles_descriptions[3]; // DS:af70
      std::array<char, 43> delay_question; // DS:af76
      std::array<char, 37> delay_reply; // DS:afa1
      std::array<char, 7> map_aerial; // DS:afc6
      std::array<char, 6> map_field; // DS:afcd
      std::array<char, 28> map_reply; // DS:afd3
      std::array<char, 3> sound_on; // DS:afef
      std::array<char, 4> sound_off; // DS:aff2
      std::array<char, 30> sound_reply; // DS:aff6
      std::array<char, 5> pictures_will; // DS:b014
      std::array<char, 9> pictures_will_not; // DS:b019
      std::array<char, 41> pictures_reply; // DS:b022
      std::array<char, 10> create_saving; // DS:b04b
      std::array<char, 9> create_file_name; // DS:b055
      std::array<char, 9> create_check_name; // DS:b05e
      std::array<char, 12> create_saved; // DS:b067
      std::array<char, 9> save_check_name; // DS:b073
      std::array<char, 9> save_recheck_name; // DS:b07c
      std::array<char, 21> save_failed; // DS:b085
      std::array<char, 3> save_failed_format; // DS:b09a
      std::array<char, 9> save_open_name; // DS:b09d
      std::array<char, 12> save_where; // DS:b0a6
      std::array<char, 7> save_confirm; // DS:b0b2
      char save_answer; // DS:b0b9
      std::array<char, 10> save_saving; // DS:b0ba
      std::array<char, 17> save_finished; // DS:b0c4
      std::array<char, 13> logo_file_name; // DS:b0d5
      std::array<char, 12> leave_question; // DS:b0e2
      std::array<char, 18> new_game_question; // DS:b0ee
      std::array<char, 10> save_quit_label_save; // DS:b100
      std::array<char, 14> save_quit_label_load; // DS:b10a
      std::array<char, 9> save_quit_label_end; // DS:b118
      std::array<char, 17> save_quit_question; // DS:b121
      std::array<FarPointer, 3> save_quit_actions; // DS:b132
      std::array<std::uint8_t, 48> save_quit_menu; // DS:b13e
      std::array<char, 14> settings_label_battles; // DS:b16e
      std::array<char, 14> settings_label_delay; // DS:b17c
      std::array<char, 12> settings_label_maps; // DS:b18a
      std::array<char, 10> settings_label_music; // DS:b196
      std::array<char, 15> settings_label_pictures; // DS:b1a0
      std::array<char, 12> settings_label_save_quit; // DS:b1af
      std::array<char, 9> settings_title; // DS:b1bb
      std::array<char, 17> settings_question; // DS:b1c4
      std::uint8_t settings_padding_b1d5; // DS:b1d5
      std::array<FarPointer, 6> settings_actions; // DS:b1d6
      std::array<std::uint8_t, 84> settings_menu; // DS:b1ee
    };
  };
  union {
    std::array<std::uint8_t, 146> combat_text; // DS:b242..b2d4
    struct {
      std::array<char, 13> text_cannot_move; // DS:b242
      std::array<char, 14> text_cannot_fight; // DS:b24f
      std::array<char, 10> text_defenders; // DS:b25d
      std::array<char, 32> supply_notice_format; // DS:b267
      std::array<char, 25> battle_siege_stalemate_text; // DS:b287
      std::array<char, 27> battle_victory_format; // DS:b2a0
      std::array<char, 25> battle_stalemate_text; // DS:b2bb
    };
  };
  union {
    std::array<std::uint8_t, 42> btlready_data; // DS:b2d4..b2fe
    struct {
      std::uint8_t                 battle_ready_blank; // DS:b2d4
      std::array<std::uint8_t, 1>  unknown_b2d5;
      std::array<std::uint8_t, 13> battle_ready_name_format; // DS:b2d6
      std::array<std::uint8_t, 27> battle_ready_text; // DS:b2e3
    };
  };
  std::array<std::uint8_t, 20> battle_round_format; // DS:b2fe: "TURN %d", the round banner
  union {
    std::array<std::uint8_t, 60> btlcalc_data; // DS:b312..b34e
    struct {
      std::array<std::uint8_t, 8>  btlcalc_skill_class;   // DS:b312
      std::array<std::uint8_t, 44> btlcalc_terrain_class; // DS:b31a
      std::array<std::uint8_t, 4>  btlcalc_step_x;        // DS:b346
      std::array<std::uint8_t, 4>  btlcalc_step_y;        // DS:b34a
    };
  };
  union {
    std::array<std::uint8_t, 3842> battle_text; // DS:b34e..c250
    struct {
      std::array<char, 22>          battle_fmt_garrison;        // DS:b34e
      std::array<char, 17>          battle_fmt_title;           // DS:b364
      std::array<char, 10>          battle_fmt_ok;              // DS:b375
      std::array<char, 10>          battle_fmt_damage;          // DS:b37f
      std::array<char, 10>          battle_fmt_morale;          // DS:b389
      std::array<char, 10>          battle_fmt_training;        // DS:b393
      std::array<char, 10>          battle_fmt_fatigue;         // DS:b39d
      std::array<char, 9>           battle_fmt_fuel;            // DS:b3a7
      std::array<char, 9>           battle_fmt_ammo;            // DS:b3b0
      std::array<char, 13>          battle_fmt_no_orders;       // DS:b3b9
      std::array<char, 13>          battle_fmt_orders;          // DS:b3c6
      std::array<char, 10>          battle_fmt_target;          // DS:b3d3
      std::array<char, 14>          battle_fmt_apat;            // DS:b3dd
      std::array<char, 10>          battle_fmt_defense;         // DS:b3eb
      std::array<char, 10>          battle_fmt_speed;           // DS:b3f5
      std::array<char, 10>          battle_fmt_range;           // DS:b3ff
      std::array<char, 4>           battle_fmt_end;             // DS:b409
      std::uint8_t                  unknown_b40d;               // DS:b40d
      FarPointer                    battle_handlers;            // DS:b40e
      FarPointer                    far_code_b412;              // DS:b412
      FarPointer                    far_code_b416;              // DS:b416
      FarPointer                    far_code_b41a;              // DS:b41a
      FarPointer                    far_code_b41e;              // DS:b41e
      FarPointer                    far_code_b422;              // DS:b422
      FarPointer                    far_code_b426;              // DS:b426
      FarPointer                    far_code_b42a;              // DS:b42a
      FarPointer                    far_code_b42e;              // DS:b42e
      FarPointer                    far_code_b432;              // DS:b432
      std::array<std::uint8_t, 2>   sprite_variant;             // DS:b436
      std::array<std::uint8_t, 4>   battle_info_widths;         // DS:b438
      std::array<std::uint8_t, 12>  battle_top_widths;          // DS:b43c
      std::array<std::uint8_t, 6>   battle_mid_left_widths;     // DS:b448
      std::array<std::uint8_t, 6>   battle_mid_right_widths;    // DS:b44e
      std::array<std::uint8_t, 4>   battle_left_mark;           // DS:b454
      std::array<std::uint8_t, 5>   battle_low_left_widths;     // DS:b458
      std::array<std::uint8_t, 5>   battle_right_mark;          // DS:b45d
      std::array<std::uint8_t, 12>  battle_low_right_widths;    // DS:b462
      std::array<char, 6>           battle_commander_format;    // DS:b46e
      std::uint16_t                 battle_division_format;     // DS:b474
      std::array<std::uint8_t, 13>  battle_wait_format;         // DS:b476
      std::array<char, 13>          battle_prompt_format;       // DS:b483
      std::array<char, 11>          battle_fmt_sighted;         // DS:b490
      std::array<char, 15>          battle_text_sighted;        // DS:b49b
      std::uint16_t                 hexmap_offsets[6];          // DS:b4aa
      std::array<std::uint8_t, 43>  hex_terrain_codes;          // DS:b4b6
      std::array<std::uint8_t, 10>  battle_hurricane_text;      // DS:b4e1
      std::array<char, 4>           battle_title_style;         // DS:b4eb
      std::array<char, 27>          battle_title_format;        // DS:b4ef
      std::array<char, 19>          battle_place_format;        // DS:b50a
      std::array<char, 19>          battle_terrain_format;      // DS:b51d
      std::array<char, 7>           battle_title_end;           // DS:b530
      std::array<std::uint8_t, 9>   unknown_b537;               // DS:b537
      ListWindow                    deploy_window;              // DS:b540: the element list of the battle deployment
      std::array<char, 30>          deploy_header;              // DS:b553
      std::array<char, 4>           deploy_mark;                // DS:b571
      std::array<char, 13>          deploy_name;                // DS:b575
      std::array<char, 33>          deploy_placed;              // DS:b582
      std::array<char, 31>          deploy_waiting;             // DS:b5a3
      std::array<char, 39>          deploy_prompt;              // DS:b5c2
      std::array<char, 33>          deploy_refused;             // DS:b5e9
      std::array<std::uint8_t, 8>   unknown_b60a;               // DS:b60a
      std::array<std::uint8_t, 24>  deploy_title;               // DS:b612
      std::array<char, 14>          deploy_unit_title;          // DS:b62a
      std::array<char, 18>          deploy_garrison_title;      // DS:b638
      std::array<char, 10>          deploy_all;                 // DS:b64a
      std::array<char, 16>          deploy_others;              // DS:b654
      std::array<char, 40>          deploy_ask_auto;            // DS:b664
      std::array<char, 32>          deploy_ask_done;            // DS:b68c
      // DS:b6ac: the deployment templates, 14 x 14 cells by row: approach direction << 4 | zone code (map/dplauto)
      std::array<std::array<std::uint8_t, 14>, 14> deploy_templates;
      // DS:b770: the template zone codes in turn, by table: type mode above 1 (or not) on a straight (or diagonal)
      // approach
      std::array<std::array<std::uint8_t, 8>, 4> deploy_zone_codes;
      // DS:b790: per battle cell (by row), a bit per approach direction from which an attacker may deploy there
      std::array<std::array<std::uint8_t, 14>, 14> deploy_zone_bits;
      // DS:b854: per approach direction, the attacker's 4 x 4 scan: x step, y step, first x, first y
      std::array<std::array<std::uint8_t, 4>, 8> deploy_scan;
      // DS:b874: per facing, the five cells tried around a garrison's element of type mode above 1: a signed x nibble
      // << 4 | a signed y nibble
      std::array<std::array<std::uint8_t, 5>, 8> deploy_offsets;
      std::array<std::uint8_t, 8>   hq_fmt_side;                // DS:b89c
      std::array<char, 14>          hq_city_garrison;           // DS:b8a4
      std::array<char, 17>          hq_new_title;               // DS:b8b2
      std::array<char, 27>          hq_new_prompt;              // DS:b8c3
      std::array<char, 41>          hq_fmt_moved;               // DS:b8de
      std::array<char, 11>          hq_div_killed;              // DS:b907
      std::array<char, 12>          hq_div_injured;             // DS:b912
      std::array<char, 39>          hq_fmt_division;            // DS:b91e
      std::array<char, 11>          hq_army_killed;             // DS:b945
      std::array<char, 12>          hq_army_injured;            // DS:b950
      std::array<char, 32>          hq_fmt_army;                // DS:b95c
      std::array<char, 11>          hq_intel_killed;            // DS:b97c
      std::array<char, 12>          hq_intel_injured;           // DS:b987
      std::array<char, 29>          hq_fmt_intel;               // DS:b993
      std::array<char, 11>          hq_ops_killed;              // DS:b9b0
      std::array<char, 12>          hq_ops_injured;             // DS:b9bb
      std::array<char, 27>          hq_fmt_ops;                 // DS:b9c7
      std::array<std::uint8_t, 8>   battle_ai_move_class;       // DS:b9e2
      std::array<std::uint8_t, 8>   battle_ai_kind;             // DS:b9ea
      std::array<std::uint8_t, 40>  battle_ai_offsets;          // DS:b9f2
      std::array<std::uint8_t, 20>  battle_ai_actions;          // DS:ba1a
      std::array<std::uint8_t, 16>  battle_ai_pairs;            // DS:ba2e
      std::array<std::uint8_t, 21>  battle_fmt_wiped;           // DS:ba3e
      std::array<char, 3>           battle_text_vs;             // DS:ba53
      std::array<char, 3>           battle_text_s;              // DS:ba56
      std::array<char, 7>           battle_text_bunker;         // DS:ba59
      std::array<char, 20>          battle_fmt_firing0;         // DS:ba60
      std::array<char, 20>          battle_fmt_firing1;         // DS:ba74
      std::array<std::uint8_t, 8>   battle_skill_kinds;         // DS:ba88
      std::array<std::uint8_t, 18>  mine_text;                  // DS:ba90
      std::array<char, 41>          retreat_mines_text;         // DS:baa2: the report of mines found in a retreat
      std::uint8_t                  unknown_bacb;               // DS:bacb
      std::uint8_t                  battle_cursor_x;            // DS:bacc
      std::uint8_t                  battle_cursor_y;            // DS:bacd
      std::array<std::uint8_t, 8>   battle_move_class;          // DS:bace
      std::array<std::uint8_t, 14>  battle_fmt_cursor;          // DS:bad6
      std::array<char, 6>           battle_fmt_extra;           // DS:bae4
      std::array<char, 8>           battle_fmt_cell_ok;         // DS:baea
      std::array<char, 25>          battle_msg_cannot_move;     // DS:baf2
      std::array<char, 29>          battle_msg_select_target;   // DS:bb0b
      std::array<char, 29>          battle_msg_set_target;      // DS:bb28
      std::array<char, 26>          battle_msg_no_units;        // DS:bb45
      std::array<char, 34>          battle_msg_withdraw_from;   // DS:bb5f
      std::array<char, 5>           text_bb81;                  // DS:bb81
      std::array<char, 7>           text_bb86;                  // DS:bb86
      std::array<char, 6>           text_bb8d;                  // DS:bb8d
      std::array<char, 7>           text_bb93;                  // DS:bb93
      std::array<char, 9>           text_bb9a;                  // DS:bb9a
      std::uint8_t                  unknown_bba3;               // DS:bba3
      std::array<MenuItem, 6>       battle_menu_items;  // DS:bba4: the battle orders menu, ended by a 0 label
      std::array<ScreenPoint, 5>    battle_menu_places; // DS:bbec: the keyboard menu's button places
      std::array<std::uint8_t, 21>  battle_msg_orders;          // DS:bc00
      std::array<char, 23>          battle_msg_cannot_withdraw; // DS:bc15
      std::array<char, 31>          battle_fmt_side_orders;     // DS:bc2c
      std::array<char, 24>          battle_msg_preparing;       // DS:bc4b
      std::array<char, 26>          battle_msg_orders_finished; // DS:bc63
      std::uint8_t                  unknown_bc7d;               // DS:bc7d
      std::array<std::uint8_t, 8>   battle_retreat_class;       // DS:bc7e
      std::array<std::uint8_t, 18>  dispatch_error;             // DS:bc86
      std::array<FarPointer, 135>   message_table;              // DS:bc98: the battle messages, by message id
      std::array<char, 23>          order_no_fuel;              // DS:beb4
      std::array<char, 29>          order_no_ammo;              // DS:becb
      std::array<char, 33>          order_prepared;             // DS:bee8
      std::array<char, 34>          order_ask_attack;           // DS:bf09
      std::array<char, 27>          order_begin_attack;         // DS:bf2b
      std::array<char, 22>          order_occupied;             // DS:bf46
      std::array<char, 13>          order_unit_label;           // DS:bf5c
      std::array<char, 31>          order_bridge_destroyed;     // DS:bf69
      std::array<char, 33>          mines_laid_text;            // DS:bf88
      std::array<char, 30>          mines_removed_text;         // DS:bfa9
      std::array<char, 29>          bridge_built_text;          // DS:bfc7
      std::array<char, 33>          arms_repaired_text;         // DS:bfe4
      std::array<char, 33>          unit_arms_repaired_text;    // DS:c005
      std::uint16_t                 transfer_panel_x;           // DS:c026
      std::uint16_t                 transfer_panel_y;           // DS:c028
      std::array<char, 9>           transfer_title;             // DS:c02a
      std::array<char, 15>          transfer_name_format;       // DS:c033
      std::array<char, 12>          transfer_name_text;         // DS:c042
      std::array<char, 3>           text_c04e;                  // DS:c04e
      std::array<char, 3>           text_c051;                  // DS:c051
      std::array<char, 3>           text_c054;                  // DS:c054
      std::array<char, 10>          transfer_amount_format;     // DS:c057
      std::array<char, 4>           transfer_amount_text;       // DS:c061
      std::array<char, 7>           transfer_colour;            // DS:c065
      std::uint16_t                 near_c06c;                  // DS:c06c
      std::uint16_t                 near_c06e;                  // DS:c06e
      std::uint16_t                 near_c070;                  // DS:c070
      std::array<std::uint8_t, 2>   transfer_unlimited;         // DS:c072
      std::array<std::uint8_t, 28>  transfer_prompt;            // DS:c074
      std::array<char, 27>          transfer_begin;             // DS:c090
      std::uint8_t                  unknown_c0ab;               // DS:c0ab
      std::array<char, 27>          arrived_text;               // DS:c0ac
      std::uint8_t                  unknown_c0c7;               // DS:c0c7
      std::array<FarPointer, 14>    order_handlers; // DS:c0c8: the handler of each order code
      std::uint16_t                 order_moves[14]; // DS:c100: the repeat flag of each order code
      std::array<std::uint8_t, 8>   move_class;                 // DS:c11c
      std::array<std::uint8_t, 6>   sight_ranges;               // DS:c124
      std::array<std::uint8_t, 23>  mines_found_text;           // DS:c12a
      std::array<char, 37>          minefield_text;             // DS:c141
      std::array<char, 35>          lost_sight_format;          // DS:c166
      std::uint8_t                  unknown_c189;               // DS:c189
      std::array<char, 46>          notice_command_text;        // DS:c18a
      std::array<char, 46>          notice_control_text;        // DS:c1b8
      std::array<char, 39>          notice_battle_text;         // DS:c1e6
      std::array<char, 29>          notice_attack_format;       // DS:c20d
      std::array<char, 37>          notice_under_attack_text;   // DS:c22a
      std::uint8_t                  unknown_c24f;               // DS:c24f
    };
  };
  union {
    std::array<std::uint8_t, 131> notend_data; // DS:c250..c2d3
    struct {
      std::array<std::uint8_t, 8>  notend_occupy_class; // DS:c250
      std::array<char, 19>         end_occupied_format;    // DS:c258
      std::array<char, 9>         end_garrison_name;    // DS:c26b
      std::array<char, 17>         end_loss_format;    // DS:c274
      std::array<char, 17>         end_wiped_out_text;    // DS:c285
      std::array<char, 19>         end_undamaged_text;    // DS:c296
      std::array<char, 20>         end_one_loss_format;    // DS:c2a9
      std::array<char, 22>         end_losses_format;    // DS:c2bd
    };
  };
  std::uint8_t                 unknown_c2d3; // DS:c2d3
  std::array<std::uint8_t, 2>  atopsp;       // DS:c2d4
  std::array<std::uint8_t, 2>  aexit_rtn;    // DS:c2d6
  std::array<std::uint8_t, 2>  asizds;       // DS:c2d8
  std::array<std::uint8_t, 20> nheap_desc;   // DS:c2da
  std::array<std::uint8_t, 2>  aseghi;       // DS:c2ee
  std::array<std::uint8_t, 2>  aseglo;       // DS:c2f0
  std::array<std::uint8_t, 14> acfinfo;      // DS:c2f2
  std::array<std::uint8_t, 4>  aintdiv;      // DS:c300
  std::array<std::uint8_t, 10> fac;          // DS:c304
  std::array<std::uint8_t, 2>  crt_errno;    // DS:c30e
  std::array<std::uint8_t, 2>  umaskval;     // DS:c310
  std::array<std::uint8_t, 2>  pspadr;       // DS:c312
  std::array<std::uint8_t, 2>  psp;          // DS:c314
  union {                                    // DS:c316
    std::uint8_t osversion;
    std::uint8_t dosvermajor;
    std::uint8_t osmajor;
  };
  union { // DS:c317
    std::uint8_t dosverminor;
    std::uint8_t osminor;
  };
  std::uint8_t osmode; // DS:c318
  union {              // DS:c319
    std::array<std::uint8_t, 2> oserr;
    std::array<std::uint8_t, 2> doserrno;
  };
  std::array<std::uint8_t, 2>   nfile;            // DS:c31b
  std::array<std::uint8_t, 20>  osfile;           // DS:c31d
  std::array<std::uint8_t, 2>   argc;             // DS:c331
  std::array<std::uint8_t, 2>   argv;             // DS:c333
  std::array<std::uint8_t, 2>   crt_environ;      // DS:c335
  std::array<std::uint8_t, 5>   pgmptr;           // DS:c337
  std::array<std::uint8_t, 2>   child;            // DS:c33c
  std::uint8_t                  ovlflag;          // DS:c33e
  std::uint8_t                  intno;            // DS:c33f
  std::array<std::uint8_t, 4>   ovlvec;           // DS:c340
  std::array<std::uint8_t, 2>   chbuf;            // DS:c344
  std::array<std::uint8_t, 24>  adbgmsg;          // DS:c346
  std::array<std::uint8_t, 2>   qczrinit;         // DS:c35e
  std::array<std::uint8_t, 2>   a_dbswpflg;       // DS:c360
  std::array<std::uint8_t, 2>   a_dbswpchk;       // DS:c362
  std::array<std::uint8_t, 2>   a_dbrterr;        // DS:c364
  std::array<std::uint8_t, 6>   a_dbexit;         // DS:c366
  std::array<std::uint8_t, 4>   a_dbptrchk;       // DS:c36c
  std::array<std::uint8_t, 12>  fpinit;           // DS:c370
  std::array<std::uint8_t, 2>   toward_segment;   // DS:c37c
  std::array<std::uint8_t, 2>   unknown_c37e;     // DS:c37e
  std::array<std::uint8_t, 2>   unitai_segment;   // DS:c380
  std::array<std::uint8_t, 2>   btlcurs_segment;  // DS:c382
  std::array<std::uint8_t, 2>   movecost_segment; // DS:c384
  std::array<std::uint8_t, 228> unknown_c386;     // DS:c386
  union {                                         // DS:c46a
    std::uint16_t edata;
    std::uint16_t tile_ems_handle;
  };
  std::uint16_t                 ems_pages[8];      // DS:c46c
  std::uint16_t                 frame_ems_handle;  // DS:c47c
  std::uint16_t                 sprite_ems_handle; // DS:c47e
  std::uint16_t                 numdlg_ems_handle; // DS:c480
  std::array<Person, 20>        people_backup;     // DS:c482: the staff candidates as drawn, before their rerolls
  std::uint16_t                 scenario_text_x;   // DS:c6ee
  std::uint16_t                 scenario_text_y;   // DS:c6f0
  std::uint16_t                 end_score;         // DS:c6f2
  std::array<std::uint8_t, 78>  random_state;      // DS:c6f4
  std::uint16_t                 list_mouse;        // DS:c742
  std::uint16_t                 tbl_save_map;      // DS:c744
  std::uint16_t                 tbl_save_level;    // DS:c746
  std::uint16_t                 initial_person;    // DS:c748
  std::uint16_t                 initial_template5; // DS:c74a
  std::uint16_t                 initial_template4; // DS:c74c
  std::uint16_t                 brief_mark_x;      // DS:c74e
  std::uint16_t                 brief_mark_y;      // DS:c750
  std::uint16_t                 daily_record;      // DS:c752
  std::uint8_t                  daily_side;        // DS:c754
  std::array<std::uint8_t, 1>   unknown_c755;
  std::uint8_t                  air_side; // DS:c756
  std::array<std::uint8_t, 1>   unknown_c757;
  std::uint8_t                  event_side; // DS:c758
  std::array<std::uint8_t, 1>   unknown_c759;
  std::uint16_t                 arrival_unit;    // DS:c75a
  std::uint16_t                 pending_request; // DS:c75c
  std::uint8_t                  request_x;       // DS:c75e
  std::uint8_t                  request_y;       // DS:c75f
  std::uint8_t                  request_error;   // DS:c760
  std::array<std::uint8_t, 1>   unknown_c761;
  std::uint16_t                 request_person;   // DS:c762
  std::uint16_t                 request_template; // DS:c764
  std::uint8_t                  arrival_side;     // DS:c766
  std::array<std::uint8_t, 1>   unknown_c767;
  std::uint16_t                 invalid_request; // DS:c768
  std::array<std::uint8_t, 256> flash_buffers;   // DS:c76a
  std::uint8_t                  transfer_unit;   // DS:c86a
  std::uint8_t                  transfer_group;  // DS:c86b
  std::array<std::array<char, 100>, 5> advice_messages; // DS:c86c: the staff's advice text per topic, empty when none
  union {                                        // DS:ca60
    std::uint16_t               supply_objective;
    std::array<std::uint8_t, 2> supply_objective_bytes;
  };
  std::uint16_t supply_table[2][3][3]; // DS:ca62: [party][ammunition, fuel, food][current, capacity, -]
  std::uint16_t                supply_total; // DS:ca86
  std::uint8_t                 supply_count; // DS:ca88
  std::array<std::uint8_t, 1>  unknown_ca89;
  std::uint16_t                supply_amounts[12]; // DS:ca8a
  std::array<std::uint8_t, 31> tally_first_name;   // DS:caa2
  std::array<std::uint8_t, 31> tally_second_name;  // DS:cac1
  std::array<std::uint8_t, 12> supply_types;       // DS:cae0
  std::uint8_t                 tally_reject_count; // DS:caec
  std::array<std::uint8_t, 1>  unknown_caed;
  std::uint16_t                supply_others[12]; // DS:caee
  std::uint32_t                supply_sum_b[12];  // DS:cb06
  std::uint32_t                supply_sum_a[12];  // DS:cb36
  std::uint32_t                supply_sum_c[12];  // DS:cb66
  std::array<std::uint8_t, 12> supply_avg_b;      // DS:cb96
  std::array<std::uint8_t, 12> supply_avg_a;      // DS:cba2
  std::array<std::uint8_t, 12> supply_avg_c;      // DS:cbae
  union {                                         // DS:cbba
    struct {
      std::uint16_t tally_unit;       // DS:cbba
      std::uint16_t tally_other_unit; // DS:cbbc: 0 when the unit is compared with an objective
    };
    std::uint16_t alloc_units[2]; // the two units sharing the allocation
  };
  union {                                       // DS:cbbe
    std::uint16_t tally_source;
    std::uint16_t alloc_objective;
  };
  std::array<Item, 6>           supply_spare;  // DS:cbc0; tally: the stock items of another class
  std::array<Item, 12>          alloc_slots;   // DS:cbde: the items allocated to the units, one row per type
  std::array<std::uint8_t, 158> rq_type_list;  // DS:cc1a
  std::uint8_t                  air_stat_14;   // DS:ccb8
  std::uint8_t                  air_stat_1b;   // DS:ccb9
  std::uint8_t                  air_stat_13;   // DS:ccba
  std::array<std::uint8_t, 1>   unknown_ccbb;
  std::uint16_t                 air_person;      // DS:ccbc
  std::uint16_t                 air_troops;      // DS:ccbe
  std::uint8_t                  special_charm; // DS:ccc0
  std::uint8_t                  special_bravery; // DS:ccc1
  std::uint16_t                 special_person;  // DS:ccc2
  std::uint16_t                 special_order;   // DS:ccc4
  std::uint8_t                  special_experience; // DS:ccc6
  std::uint8_t                  special_leadership; // DS:ccc7
  std::array<std::uint8_t, 520> vram;            // DS:ccc8
  std::uint8_t                  transfer_auto;   // DS:ced0
  std::array<std::uint8_t, 1>   unknown_ced1;
  union { // DS:ced2
    std::uint16_t               transfer_objective_ptr;
    std::array<std::uint8_t, 2> transfer_objective_ptr_bytes;
  };
  std::uint16_t                 transfer_table[2][3][3]; // DS:ced4: [side][ammunition, fuel, food][TransferColumn]
  std::uint16_t                 transfer_interactive; // DS:cef8
  std::array<std::uint8_t, 6>   unknown_cefa;
  std::array<std::uint8_t, 198> battle_terrain; // DS:cf00
  union {                                       // DS:cfc6
    std::uint16_t ai_unit_list[32];
    std::uint16_t supply_units[32];
    std::uint16_t row_units[32]; // the regiments of a list dialog's rows
  };
  std::array<SaveSlot, 20>      save_slots;       // DS:d006
  std::uint16_t                 icon_segment;     // DS:d0f6
  std::array<std::uint8_t, 600> battle_positions; // DS:d0f8
  union {                                         // DS:d350
    std::uint16_t tbl_objective_list[21];
    std::uint16_t view_objectives[21];
  };
  std::array<std::uint8_t, 1600> map_tiles; // DS:d37a
  union {                                   // DS:d9ba
    std::uint16_t strength_items[6];
    std::uint16_t view_stocks[6];
  };
  std::array<std::uint8_t, 1600> arrival_candidates; // DS:d9c6
  union {                                            // DS:e006
    std::uint16_t bitmap_buffer;
    std::uint16_t image_segment;
  };
  std::array<BattleLoss, 60>    battle_losses;  // DS:e008
  std::uint16_t                 sound_driver;   // DS:e224
  std::array<std::uint8_t, 100> event_buffer;   // DS:e226
  std::array<std::uint8_t, 100> request_buffer; // DS:e28a
  union {                                       // DS:e2ee
    std::uint16_t supply_window;
    std::uint16_t list_window;
  };
  std::uint8_t list_column; // DS:e2f0
  union {                   // DS:e2f1
    std::uint8_t supply_top;
    std::uint8_t list_top;
  };
  std::uint8_t list_columns; // DS:e2f2
  std::uint8_t list_rows;    // DS:e2f3
  union {                    // DS:e2f4
    std::uint8_t save_slot;
    std::uint8_t supply_row;
    std::uint8_t list_row;
  };
  std::array<std::uint8_t, 1>   unknown_e2f5;
  std::array<std::uint8_t, 200> scenario_brief; // DS:e2f6
  std::array<std::uint8_t, 600> scenario_notes; // DS:e3be
  union {                                       // DS:e616
    std::uint16_t message_picture_segment; // MSGRP.DAT, the message pictures
    std::uint16_t numdlg_image_segment;
  };
  std::uint8_t                  hexmap_index; // DS:e618
  std::array<std::uint8_t, 1>   unknown_e619;
  std::array<Element, 6>        battle_elements;       // DS:e61a: the garrison's elements, one per objective item
  std::array<std::uint8_t, 150> message_buffer;        // DS:e668
  std::uint16_t                 overview_tile_segment; // DS:e6fe
  union {                                              // DS:e700
    std::uint16_t               stream_ptr[2];
    FarPointer                  stream; // the picture decoder's read position
    std::array<std::uint8_t, 4> stream_ptr_bytes;
    std::uint16_t               gfx_source[2];
  };
  union { // DS:e704
    std::uint16_t tbl_type_list[158];
    std::uint16_t weapon_list[158];
  };
  std::uint16_t hex_display_segment; // DS:e840
  std::uint16_t vseg_segment;        // DS:e842
  std::uint16_t numdlg_save_segment; // DS:e844
  union {                            // DS:e846
    std::uint16_t ai_element_list[128];
    std::uint16_t tally_records[128];
    std::uint16_t alloc_elements[128];
    std::uint16_t row_elements[128]; // the elements of a list dialog's rows
  };
  union { // DS:e946
    std::array<std::uint8_t, 22> battle_garrison;
    std::array<std::uint8_t, 22> garrison_unit;
    Unit                         garrison; // the pseudo-unit a besieged objective fights with (core/notice)
  };
  std::uint16_t                 frame_segment;     // DS:e95c
  std::uint8_t                  battle_path_queue; // DS:e95e
  std::array<std::uint8_t, 195> unknown_e95f;
  std::uint16_t                 sprite_segment;   // DS:ea22
  std::uint16_t                 tile_segment;     // DS:ea24
  std::uint16_t                 face_segment;     // DS:ea26
  std::uint16_t                 portrait_segment; // DS:ea28
  std::uint16_t                 general_list[20]; // DS:ea2a
  union {                                         // DS:ea52
    std::uint16_t                 battle_path_costs[199];
    std::array<std::uint8_t, 398> battle_path_costs_bytes;
  };
  union { // DS:ebe0
    std::array<std::uint8_t, 5152> stack;
    std::array<std::uint8_t, 5152> end;
  };
};

#pragma pack(pop)

static_assert(offsetof(FarPointer, offset) == 0);
static_assert(offsetof(FarPointer, segment) == 2);
static_assert(sizeof(FarPointer) == 4);
static_assert(offsetof(Order, target) == 0);
static_assert(offsetof(Order, kind) == 2);
static_assert(offsetof(Order, progress) == 3);
static_assert(sizeof(Order) == 4);
static_assert(offsetof(RequestSlot, target) == 0);
static_assert(offsetof(RequestSlot, argument) == 2);
static_assert(offsetof(RequestSlot, kind) == 3);
static_assert(sizeof(RequestSlot) == 4);
static_assert(sizeof(NoonEvent) == 12);
static_assert(offsetof(Task, x) == 0);
static_assert(offsetof(Task, y) == 1);
static_assert(offsetof(Task, kind) == 2);
static_assert(offsetof(Task, progress) == 3);
static_assert(sizeof(Task) == 4);
static_assert(offsetof(Item, type) == 0);
static_assert(offsetof(Item, quantity) == 1);
static_assert(offsetof(Item, extra) == 3);
static_assert(sizeof(Item) == 5);
static_assert(sizeof(BattleLoss) == 9);
static_assert(offsetof(AirStrike, x) == 0);
static_assert(offsetof(AirStrike, y) == 1);
static_assert(offsetof(AirStrike, planes) == 2);
static_assert(offsetof(AirStrike, hours) == 4);
static_assert(offsetof(AirStrike, turns) == 5);
static_assert(sizeof(AirStrike) == 6);
static_assert(offsetof(AirDrop, x) == 0);
static_assert(offsetof(AirDrop, y) == 1);
static_assert(offsetof(AirDrop, paratroops) == 2);
static_assert(offsetof(AirDrop, transports) == 4);
static_assert(offsetof(AirDrop, hours) == 6);
static_assert(offsetof(AirDrop, due) == 7);
static_assert(sizeof(AirDrop) == 8);
static_assert(offsetof(Side, stock) == 0);
static_assert(offsetof(Side, income) == 6);
static_assert(offsetof(Side, pool_a) == 12);
static_assert(offsetof(Side, planes) == 14);
static_assert(offsetof(Side, pending_a) == 16);
static_assert(offsetof(Side, pending_planes) == 18);
static_assert(offsetof(Side, paratroops) == 20);
static_assert(offsetof(Side, strength) == 22);
static_assert(offsetof(Side, objectives) == 23);
static_assert(offsetof(Side, requests) == 23);
static_assert(offsetof(Side, strike) == 35);
static_assert(offsetof(Side, drop) == 41);
static_assert(offsetof(Side, lift) == 49);
static_assert(offsetof(AirLift, planes) == 8);
static_assert(sizeof(AirLift) == 12);
static_assert(offsetof(Side, order) == 61);
static_assert(sizeof(Side) == 65);
static_assert(offsetof(Person, name) == 0);
static_assert(offsetof(Person, status) == 18);
static_assert(offsetof(Person, leadership) == 19);
static_assert(offsetof(Person, charm) == 20);
static_assert(offsetof(Person, bravery) == 21);
static_assert(offsetof(Person, health) == 22);
static_assert(offsetof(Person, byte_17) == 23);
static_assert(offsetof(Person, infantry) == 24);
static_assert(offsetof(Person, artillery) == 25);
static_assert(offsetof(Person, armor) == 26);
static_assert(offsetof(Person, experience) == 27);
static_assert(offsetof(Person, assignment) == 28);
static_assert(offsetof(Person, portrait) == 29);
static_assert(offsetof(Person, role) == 30);
static_assert(offsetof(Person, flags) == 30);
static_assert(sizeof(Person) == 31);
static_assert(offsetof(Group, tasks) == 0);
static_assert(offsetof(Group, flags) == 16);
static_assert(sizeof(Group) == 17);
static_assert(offsetof(Element, next) == 0);
static_assert(offsetof(Element, type) == 2);
static_assert(offsetof(Element, quantity) == 3);
static_assert(offsetof(Element, damage) == 5);
static_assert(offsetof(Element, position) == 7);
static_assert(offsetof(Element, cell) == 7);
static_assert(offsetof(Element, morale) == 9);
static_assert(offsetof(Element, training) == 10);
static_assert(offsetof(Element, experience) == 11);
static_assert(offsetof(Element, flags) == 12);
static_assert(sizeof(Element) == 13);
static_assert(offsetof(Unit, first) == 0);
static_assert(offsetof(Unit, x) == 2);
static_assert(offsetof(Unit, y) == 3);
static_assert(offsetof(Unit, target_x) == 4);
static_assert(offsetof(Unit, target_y) == 5);
static_assert(offsetof(Unit, resources) == 6);
static_assert(offsetof(Unit, resource_bytes) == 6);
static_assert(offsetof(Unit, orders) == 12);
static_assert(offsetof(Unit, moved) == 20);
static_assert(offsetof(Unit, flags) == 21);
static_assert(sizeof(Unit) == 22);
static_assert(offsetof(Objective, name) == 0);
static_assert(offsetof(Objective, active) == 0);
static_assert(offsetof(Objective, x) == 21);
static_assert(offsetof(Objective, y) == 22);
static_assert(offsetof(Objective, resources) == 23);
static_assert(offsetof(Objective, reserves) == 29);
static_assert(offsetof(Objective, strength) == 35);
static_assert(offsetof(Objective, strength_limit) == 36);
static_assert(offsetof(Objective, items) == 37);
static_assert(offsetof(Objective, flags) == 67);
static_assert(sizeof(Objective) == 68);
static_assert(offsetof(ElementType, name) == 0);
static_assert(offsetof(ElementType, sides) == 2);
static_assert(offsetof(ElementType, date) == 3);
static_assert(offsetof(ElementType, category) == 4);
static_assert(offsetof(ElementType, soft_rating) == 5);
static_assert(offsetof(ElementType, hard_rating) == 6);
static_assert(offsetof(ElementType, range) == 7);
static_assert(offsetof(ElementType, speed) == 8);
static_assert(offsetof(ElementType, usage) == 9);
static_assert(offsetof(ElementType, weight) == 10);
static_assert(offsetof(ElementType, mode) == 11);
static_assert(offsetof(ElementType, flags) == 12);
static_assert(sizeof(ElementType) == 13);
static_assert(offsetof(DailyRecord, name) == 0);
static_assert(offsetof(DailyRecord, flags) == 2);
static_assert(offsetof(DailyRecord, date) == 3);
static_assert(offsetof(DailyRecord, kind) == 4);
static_assert(offsetof(DailyRecord, unknown_05) == 5);
static_assert(offsetof(DailyRecord, state) == 12);
static_assert(sizeof(DailyRecord) == 13);
static_assert(offsetof(RequestConfig, target) == 0);
static_assert(offsetof(RequestConfig, argument) == 2);
static_assert(offsetof(RequestConfig, kind) == 3);
static_assert(offsetof(RequestConfig, delay) == 4);
static_assert(offsetof(RequestConfig, enabled) == 5);
static_assert(offsetof(RequestConfig, rate0) == 11);
static_assert(offsetof(RequestConfig, rate1) == 12);
static_assert(sizeof(RequestConfig) == 13);
static_assert(offsetof(SaveSlot, scenario) == 0);
static_assert(offsetof(SaveSlot, delivery) == 1);
static_assert(offsetof(SaveSlot, year) == 2);
static_assert(offsetof(SaveSlot, month) == 3);
static_assert(offsetof(SaveSlot, day) == 4);
static_assert(offsetof(SaveSlot, hour) == 5);
static_assert(offsetof(SaveSlot, control) == 6);
static_assert(offsetof(SaveSlot, checksum) == 10);
static_assert(sizeof(SaveSlot) == 12);

static_assert(sizeof(DataSegment) == 0x10000);
static_assert(offsetof(DataSegment, unknown_0000) == 0x0000);
static_assert(offsetof(DataSegment, scenario_header) == 0x0042);
static_assert(offsetof(DataSegment, scenario_first) == 0x0042);
static_assert(offsetof(DataSegment, scenario) == 0x0043);
static_assert(offsetof(DataSegment, year) == 0x0044);
static_assert(offsetof(DataSegment, month) == 0x0045);
static_assert(offsetof(DataSegment, day) == 0x0046);
static_assert(offsetof(DataSegment, hour) == 0x0047);
static_assert(offsetof(DataSegment, weather) == 0x0048);
static_assert(offsetof(DataSegment, map_scale) == 0x0049);
static_assert(offsetof(DataSegment, message_delay) == 0x004a);
static_assert(offsetof(DataSegment, message_speed) == 0x004a);
static_assert(offsetof(DataSegment, delivery_level) == 0x004b);
static_assert(offsetof(DataSegment, map_mode) == 0x004c);
static_assert(offsetof(DataSegment, arrival_mode) == 0x004c);
static_assert(offsetof(DataSegment, event_pictures) == 0x004e);
static_assert(offsetof(DataSegment, message_pictures) == 0x004e);
static_assert(offsetof(DataSegment, sound_enabled) == 0x0050);
static_assert(offsetof(DataSegment, sound_enabled_bytes) == 0x0050);
static_assert(offsetof(DataSegment, battle_view) == 0x0052);
static_assert(offsetof(DataSegment, unknown_0054) == 0x0054);
static_assert(offsetof(DataSegment, side_flags) == 0x0056);
static_assert(offsetof(DataSegment, side_delays) == 0x0058);
static_assert(offsetof(DataSegment, counter0) == 0x0058);
static_assert(offsetof(DataSegment, counter1) == 0x0059);
static_assert(offsetof(DataSegment, side_status) == 0x005a);
static_assert(offsetof(DataSegment, side_control) == 0x005e);
static_assert(offsetof(DataSegment, side_control_bytes) == 0x005e);
static_assert(offsetof(DataSegment, current_side) == 0x0062);
static_assert(offsetof(DataSegment, unknown_0063) == 0x0063);
static_assert(offsetof(DataSegment, current_side_ptr) == 0x0064);
static_assert(offsetof(DataSegment, current_group) == 0x0066);
static_assert(offsetof(DataSegment, unknown_0067) == 0x0067);
static_assert(offsetof(DataSegment, current_group_ptr) == 0x0068);
static_assert(offsetof(DataSegment, current_entry) == 0x006a);
static_assert(offsetof(DataSegment, unknown_006b) == 0x006b);
static_assert(offsetof(DataSegment, current_unit_ptr) == 0x006c);
static_assert(offsetof(DataSegment, current_unit_ptr_bytes) == 0x006c);
static_assert(offsetof(DataSegment, context_record1) == 0x006e);
static_assert(offsetof(DataSegment, context_record2) == 0x0070);
static_assert(offsetof(DataSegment, context_record2_bytes) == 0x0070);
static_assert(offsetof(DataSegment, context_record3) == 0x0072);
static_assert(offsetof(DataSegment, context_record3_bytes) == 0x0072);
static_assert(offsetof(DataSegment, context_group_record) == 0x0074);
static_assert(offsetof(DataSegment, context_group_record_bytes) == 0x0074);
static_assert(offsetof(DataSegment, terrain_ratings) == 0x0076);
static_assert(offsetof(DataSegment, move_costs) == 0x0076);
static_assert(offsetof(DataSegment, weather_odds) == 0x00ae);
static_assert(offsetof(DataSegment, side_masks) == 0x013e);
static_assert(offsetof(DataSegment, side_stance) == 0x0140);
static_assert(offsetof(DataSegment, special) == 0x0142);
static_assert(offsetof(DataSegment, request_configs) == 0x0142);
static_assert(offsetof(DataSegment, request_weights1) == 0x015c);
static_assert(offsetof(DataSegment, request_directions) == 0x01fa);
static_assert(offsetof(DataSegment, sides) == 0x021a);
static_assert(offsetof(DataSegment, people) == 0x029c);
static_assert(offsetof(DataSegment, groups) == 0x0774);
static_assert(offsetof(DataSegment, elements) == 0x07fc);
static_assert(offsetof(DataSegment, units) == 0x14fc);
static_assert(offsetof(DataSegment, objectives) == 0x1a7c);
static_assert(offsetof(DataSegment, scenario_last) == 0x2010);
static_assert(offsetof(DataSegment, scenario_after) == 0x2011);
static_assert(offsetof(DataSegment, turn_cursor) == 0x2011);
static_assert(offsetof(DataSegment, turn_order) == 0x2012);
static_assert(offsetof(DataSegment, side_entry_values) == 0x2052);
static_assert(offsetof(DataSegment, objective_links) == 0x2052);
static_assert(offsetof(DataSegment, unit_links) == 0x20a6);
static_assert(offsetof(DataSegment, map_cells) == 0x20b6);
static_assert(offsetof(DataSegment, influence_map) == 0x26f6);
static_assert(offsetof(DataSegment, unit_type_names) == 0x27be);
static_assert(offsetof(DataSegment, save_last) == 0x27be);
static_assert(offsetof(DataSegment, save_end) == 0x27bf);
static_assert(offsetof(DataSegment, text_27c0) == 0x27c0);
static_assert(offsetof(DataSegment, text_27cb) == 0x27cb);
static_assert(offsetof(DataSegment, text_27d7) == 0x27d7);
static_assert(offsetof(DataSegment, text_27e5) == 0x27e5);
static_assert(offsetof(DataSegment, text_27f3) == 0x27f3);
static_assert(offsetof(DataSegment, text_27fd) == 0x27fd);
static_assert(offsetof(DataSegment, text_280b) == 0x280b);
static_assert(offsetof(DataSegment, text_2817) == 0x2817);
static_assert(offsetof(DataSegment, text_2824) == 0x2824);
static_assert(offsetof(DataSegment, text_282d) == 0x282d);
static_assert(offsetof(DataSegment, text_2837) == 0x2837);
static_assert(offsetof(DataSegment, text_2842) == 0x2842);
static_assert(offsetof(DataSegment, text_284d) == 0x284d);
static_assert(offsetof(DataSegment, text_2859) == 0x2859);
static_assert(offsetof(DataSegment, text_2865) == 0x2865);
static_assert(offsetof(DataSegment, text_2871) == 0x2871);
static_assert(offsetof(DataSegment, text_287d) == 0x287d);
static_assert(offsetof(DataSegment, text_2889) == 0x2889);
static_assert(offsetof(DataSegment, text_2894) == 0x2894);
static_assert(offsetof(DataSegment, text_28a0) == 0x28a0);
static_assert(offsetof(DataSegment, text_28ac) == 0x28ac);
static_assert(offsetof(DataSegment, text_28b7) == 0x28b7);
static_assert(offsetof(DataSegment, text_28c2) == 0x28c2);
static_assert(offsetof(DataSegment, text_28cc) == 0x28cc);
static_assert(offsetof(DataSegment, text_28d6) == 0x28d6);
static_assert(offsetof(DataSegment, text_28de) == 0x28de);
static_assert(offsetof(DataSegment, text_28e7) == 0x28e7);
static_assert(offsetof(DataSegment, text_28f1) == 0x28f1);
static_assert(offsetof(DataSegment, text_28fb) == 0x28fb);
static_assert(offsetof(DataSegment, text_2908) == 0x2908);
static_assert(offsetof(DataSegment, text_2916) == 0x2916);
static_assert(offsetof(DataSegment, text_291f) == 0x291f);
static_assert(offsetof(DataSegment, text_292a) == 0x292a);
static_assert(offsetof(DataSegment, text_2936) == 0x2936);
static_assert(offsetof(DataSegment, text_293e) == 0x293e);
static_assert(offsetof(DataSegment, text_2948) == 0x2948);
static_assert(offsetof(DataSegment, text_2952) == 0x2952);
static_assert(offsetof(DataSegment, text_295b) == 0x295b);
static_assert(offsetof(DataSegment, text_2962) == 0x2962);
static_assert(offsetof(DataSegment, text_296b) == 0x296b);
static_assert(offsetof(DataSegment, text_2974) == 0x2974);
static_assert(offsetof(DataSegment, text_297a) == 0x297a);
static_assert(offsetof(DataSegment, text_2981) == 0x2981);
static_assert(offsetof(DataSegment, text_298a) == 0x298a);
static_assert(offsetof(DataSegment, text_2992) == 0x2992);
static_assert(offsetof(DataSegment, text_299a) == 0x299a);
static_assert(offsetof(DataSegment, text_29a2) == 0x29a2);
static_assert(offsetof(DataSegment, text_29aa) == 0x29aa);
static_assert(offsetof(DataSegment, text_29b5) == 0x29b5);
static_assert(offsetof(DataSegment, text_29c0) == 0x29c0);
static_assert(offsetof(DataSegment, text_29cb) == 0x29cb);
static_assert(offsetof(DataSegment, text_29d6) == 0x29d6);
static_assert(offsetof(DataSegment, text_29e2) == 0x29e2);
static_assert(offsetof(DataSegment, text_29ed) == 0x29ed);
static_assert(offsetof(DataSegment, text_29f9) == 0x29f9);
static_assert(offsetof(DataSegment, text_2a03) == 0x2a03);
static_assert(offsetof(DataSegment, text_2a0c) == 0x2a0c);
static_assert(offsetof(DataSegment, text_2a0f) == 0x2a0f);
static_assert(offsetof(DataSegment, text_2a16) == 0x2a16);
static_assert(offsetof(DataSegment, text_2a26) == 0x2a26);
static_assert(offsetof(DataSegment, text_2a2a) == 0x2a2a);
static_assert(offsetof(DataSegment, text_2a30) == 0x2a30);
static_assert(offsetof(DataSegment, text_2a36) == 0x2a36);
static_assert(offsetof(DataSegment, text_2a41) == 0x2a41);
static_assert(offsetof(DataSegment, text_2a4b) == 0x2a4b);
static_assert(offsetof(DataSegment, text_2a59) == 0x2a59);
static_assert(offsetof(DataSegment, text_2a61) == 0x2a61);
static_assert(offsetof(DataSegment, text_2a65) == 0x2a65);
static_assert(offsetof(DataSegment, text_2a6e) == 0x2a6e);
static_assert(offsetof(DataSegment, text_2a7e) == 0x2a7e);
static_assert(offsetof(DataSegment, text_2a8e) == 0x2a8e);
static_assert(offsetof(DataSegment, text_2a98) == 0x2a98);
static_assert(offsetof(DataSegment, text_2aa3) == 0x2aa3);
static_assert(offsetof(DataSegment, text_2aae) == 0x2aae);
static_assert(offsetof(DataSegment, text_2ab9) == 0x2ab9);
static_assert(offsetof(DataSegment, text_2ac5) == 0x2ac5);
static_assert(offsetof(DataSegment, text_2ad2) == 0x2ad2);
static_assert(offsetof(DataSegment, text_2ade) == 0x2ade);
static_assert(offsetof(DataSegment, text_2aeb) == 0x2aeb);
static_assert(offsetof(DataSegment, text_2af4) == 0x2af4);
static_assert(offsetof(DataSegment, text_2afe) == 0x2afe);
static_assert(offsetof(DataSegment, text_2b09) == 0x2b09);
static_assert(offsetof(DataSegment, text_2b13) == 0x2b13);
static_assert(offsetof(DataSegment, text_2b1e) == 0x2b1e);
static_assert(offsetof(DataSegment, text_2b2a) == 0x2b2a);
static_assert(offsetof(DataSegment, text_2b38) == 0x2b38);
static_assert(offsetof(DataSegment, text_2b45) == 0x2b45);
static_assert(offsetof(DataSegment, text_2b49) == 0x2b49);
static_assert(offsetof(DataSegment, text_2b52) == 0x2b52);
static_assert(offsetof(DataSegment, text_2b5a) == 0x2b5a);
static_assert(offsetof(DataSegment, text_2b63) == 0x2b63);
static_assert(offsetof(DataSegment, text_2b6d) == 0x2b6d);
static_assert(offsetof(DataSegment, text_2b77) == 0x2b77);
static_assert(offsetof(DataSegment, text_2b81) == 0x2b81);
static_assert(offsetof(DataSegment, text_2b88) == 0x2b88);
static_assert(offsetof(DataSegment, text_2b8e) == 0x2b8e);
static_assert(offsetof(DataSegment, text_2b99) == 0x2b99);
static_assert(offsetof(DataSegment, text_2ba7) == 0x2ba7);
static_assert(offsetof(DataSegment, text_2bb2) == 0x2bb2);
static_assert(offsetof(DataSegment, text_2bc0) == 0x2bc0);
static_assert(offsetof(DataSegment, text_2bcf) == 0x2bcf);
static_assert(offsetof(DataSegment, text_2bdc) == 0x2bdc);
static_assert(offsetof(DataSegment, text_2be5) == 0x2be5);
static_assert(offsetof(DataSegment, text_2bf3) == 0x2bf3);
static_assert(offsetof(DataSegment, text_2bf7) == 0x2bf7);
static_assert(offsetof(DataSegment, text_2c01) == 0x2c01);
static_assert(offsetof(DataSegment, text_2c08) == 0x2c08);
static_assert(offsetof(DataSegment, text_2c12) == 0x2c12);
static_assert(offsetof(DataSegment, text_2c1d) == 0x2c1d);
static_assert(offsetof(DataSegment, text_2c26) == 0x2c26);
static_assert(offsetof(DataSegment, text_2c31) == 0x2c31);
static_assert(offsetof(DataSegment, text_2c3b) == 0x2c3b);
static_assert(offsetof(DataSegment, text_2c49) == 0x2c49);
static_assert(offsetof(DataSegment, text_2c4d) == 0x2c4d);
static_assert(offsetof(DataSegment, text_2c51) == 0x2c51);
static_assert(offsetof(DataSegment, text_2c56) == 0x2c56);
static_assert(offsetof(DataSegment, text_2c5e) == 0x2c5e);
static_assert(offsetof(DataSegment, text_2c65) == 0x2c65);
static_assert(offsetof(DataSegment, text_2c69) == 0x2c69);
static_assert(offsetof(DataSegment, text_2c6d) == 0x2c6d);
static_assert(offsetof(DataSegment, text_2c72) == 0x2c72);
static_assert(offsetof(DataSegment, text_2c77) == 0x2c77);
static_assert(offsetof(DataSegment, text_2c7d) == 0x2c7d);
static_assert(offsetof(DataSegment, text_2c83) == 0x2c83);
static_assert(offsetof(DataSegment, text_2c89) == 0x2c89);
static_assert(offsetof(DataSegment, text_2c90) == 0x2c90);
static_assert(offsetof(DataSegment, text_2c97) == 0x2c97);
static_assert(offsetof(DataSegment, text_2c9f) == 0x2c9f);
static_assert(offsetof(DataSegment, text_2ca7) == 0x2ca7);
static_assert(offsetof(DataSegment, text_2caf) == 0x2caf);
static_assert(offsetof(DataSegment, text_2cb7) == 0x2cb7);
static_assert(offsetof(DataSegment, text_2cc0) == 0x2cc0);
static_assert(offsetof(DataSegment, text_2ccb) == 0x2ccb);
static_assert(offsetof(DataSegment, text_2cd6) == 0x2cd6);
static_assert(offsetof(DataSegment, general_names) == 0x2ce0);
static_assert(offsetof(DataSegment, text_2ce0) == 0x2ce0);
static_assert(offsetof(DataSegment, text_2ce6) == 0x2ce6);
static_assert(offsetof(DataSegment, text_2ced) == 0x2ced);
static_assert(offsetof(DataSegment, text_2cf2) == 0x2cf2);
static_assert(offsetof(DataSegment, text_2cf7) == 0x2cf7);
static_assert(offsetof(DataSegment, text_2cfb) == 0x2cfb);
static_assert(offsetof(DataSegment, text_2d02) == 0x2d02);
static_assert(offsetof(DataSegment, text_2d07) == 0x2d07);
static_assert(offsetof(DataSegment, text_2d0e) == 0x2d0e);
static_assert(offsetof(DataSegment, text_2d12) == 0x2d12);
static_assert(offsetof(DataSegment, text_2d16) == 0x2d16);
static_assert(offsetof(DataSegment, text_2d1a) == 0x2d1a);
static_assert(offsetof(DataSegment, text_2d1e) == 0x2d1e);
static_assert(offsetof(DataSegment, text_2d22) == 0x2d22);
static_assert(offsetof(DataSegment, text_2d26) == 0x2d26);
static_assert(offsetof(DataSegment, text_2d31) == 0x2d31);
static_assert(offsetof(DataSegment, text_2d3b) == 0x2d3b);
static_assert(offsetof(DataSegment, text_2d46) == 0x2d46);
static_assert(offsetof(DataSegment, text_2d54) == 0x2d54);
static_assert(offsetof(DataSegment, text_2d63) == 0x2d63);
static_assert(offsetof(DataSegment, text_2d73) == 0x2d73);
static_assert(offsetof(DataSegment, text_2d88) == 0x2d88);
static_assert(offsetof(DataSegment, text_2d8f) == 0x2d8f);
static_assert(offsetof(DataSegment, text_2d96) == 0x2d96);
static_assert(offsetof(DataSegment, text_2d9d) == 0x2d9d);
static_assert(offsetof(DataSegment, text_2da4) == 0x2da4);
static_assert(offsetof(DataSegment, text_2dac) == 0x2dac);
static_assert(offsetof(DataSegment, text_2db4) == 0x2db4);
static_assert(offsetof(DataSegment, text_2dbb) == 0x2dbb);
static_assert(offsetof(DataSegment, text_2dc2) == 0x2dc2);
static_assert(offsetof(DataSegment, text_2dce) == 0x2dce);
static_assert(offsetof(DataSegment, text_2dd3) == 0x2dd3);
static_assert(offsetof(DataSegment, text_2dd8) == 0x2dd8);
static_assert(offsetof(DataSegment, text_2ddf) == 0x2ddf);
static_assert(offsetof(DataSegment, text_2de6) == 0x2de6);
static_assert(offsetof(DataSegment, text_2dec) == 0x2dec);
static_assert(offsetof(DataSegment, text_2df2) == 0x2df2);
static_assert(offsetof(DataSegment, text_2dfc) == 0x2dfc);
static_assert(offsetof(DataSegment, text_2e03) == 0x2e03);
static_assert(offsetof(DataSegment, text_2e08) == 0x2e08);
static_assert(offsetof(DataSegment, text_2e0f) == 0x2e0f);
static_assert(offsetof(DataSegment, text_2e16) == 0x2e16);
static_assert(offsetof(DataSegment, text_2e1d) == 0x2e1d);
static_assert(offsetof(DataSegment, text_2e22) == 0x2e22);
static_assert(offsetof(DataSegment, text_2e27) == 0x2e27);
static_assert(offsetof(DataSegment, text_2e2e) == 0x2e2e);
static_assert(offsetof(DataSegment, text_2e35) == 0x2e35);
static_assert(offsetof(DataSegment, text_2e3c) == 0x2e3c);
static_assert(offsetof(DataSegment, text_2e41) == 0x2e41);
static_assert(offsetof(DataSegment, text_2e46) == 0x2e46);
static_assert(offsetof(DataSegment, text_2e4d) == 0x2e4d);
static_assert(offsetof(DataSegment, text_2e55) == 0x2e55);
static_assert(offsetof(DataSegment, text_2e5c) == 0x2e5c);
static_assert(offsetof(DataSegment, text_2e63) == 0x2e63);
static_assert(offsetof(DataSegment, text_2e6a) == 0x2e6a);
static_assert(offsetof(DataSegment, text_2e71) == 0x2e71);
static_assert(offsetof(DataSegment, text_2e78) == 0x2e78);
static_assert(offsetof(DataSegment, text_2e7d) == 0x2e7d);
static_assert(offsetof(DataSegment, text_2e82) == 0x2e82);
static_assert(offsetof(DataSegment, text_2e87) == 0x2e87);
static_assert(offsetof(DataSegment, resource_filenames) == 0x2e8c);
static_assert(offsetof(DataSegment, scenario_filename) == 0x2e8c);
static_assert(offsetof(DataSegment, face_file_name) == 0x2e94);
static_assert(offsetof(DataSegment, unknown_2e9d) == 0x2e9d);
static_assert(offsetof(DataSegment, end_data_name) == 0x2e9e);
static_assert(offsetof(DataSegment, unknown_2ea9) == 0x2ea9);
static_assert(offsetof(DataSegment, text_2eaa) == 0x2eaa);
static_assert(offsetof(DataSegment, unknown_2eb7) == 0x2eb7);
static_assert(offsetof(DataSegment, scensel_file) == 0x2eb8);
static_assert(offsetof(DataSegment, unknown_2ec3) == 0x2ec3);
static_assert(offsetof(DataSegment, mgamen_filename) == 0x2ec4);
static_assert(offsetof(DataSegment, unknown_2ecf) == 0x2ecf);
static_assert(offsetof(DataSegment, arms_file_name) == 0x2ed0);
static_assert(offsetof(DataSegment, unknown_2ed9) == 0x2ed9);
static_assert(offsetof(DataSegment, midmap_filename) == 0x2eda);
static_assert(offsetof(DataSegment, unknown_2ee5) == 0x2ee5);
static_assert(offsetof(DataSegment, bigmap_filename) == 0x2ee6);
static_assert(offsetof(DataSegment, unknown_2ef1) == 0x2ef1);
static_assert(offsetof(DataSegment, map_filename) == 0x2ef2);
static_assert(offsetof(DataSegment, unknown_2efd) == 0x2efd);
static_assert(offsetof(DataSegment, mappno_filename) == 0x2efe);
static_assert(offsetof(DataSegment, unknown_2f09) == 0x2f09);
static_assert(offsetof(DataSegment, hexmap_filename) == 0x2f0a);
static_assert(offsetof(DataSegment, unknown_2f15) == 0x2f15);
static_assert(offsetof(DataSegment, hexdisp_filename) == 0x2f16);
static_assert(offsetof(DataSegment, hexunit_filename) == 0x2f22);
static_assert(offsetof(DataSegment, hexfont_filename) == 0x2f2e);
static_assert(offsetof(DataSegment, hexfonts_filename) == 0x2f3a);
static_assert(offsetof(DataSegment, unknown_2f47) == 0x2f47);
static_assert(offsetof(DataSegment, msgrp_filename) == 0x2f48);
static_assert(offsetof(DataSegment, event_file_name) == 0x2f52);
static_assert(offsetof(DataSegment, text_2f5c) == 0x2f5c);
static_assert(offsetof(DataSegment, unknown_2f65) == 0x2f65);
static_assert(offsetof(DataSegment, text_2f66) == 0x2f66);
static_assert(offsetof(DataSegment, turn_variables) == 0x2f6e);
static_assert(offsetof(DataSegment, picture_handle) == 0x2f6e);
static_assert(offsetof(DataSegment, portrait_handle) == 0x2f70);
static_assert(offsetof(DataSegment, portrait_handle_bytes) == 0x2f70);
static_assert(offsetof(DataSegment, face_handle) == 0x2f72);
static_assert(offsetof(DataSegment, message_picture_handle) == 0x2f74);
static_assert(offsetof(DataSegment, unknown_2f76) == 0x2f76);
static_assert(offsetof(DataSegment, outcome_side) == 0x2f82);
static_assert(offsetof(DataSegment, outcome_reason) == 0x2f83);
static_assert(offsetof(DataSegment, daily_pending) == 0x2f84);
static_assert(offsetof(DataSegment, daily_active) == 0x2f86);
static_assert(offsetof(DataSegment, side_initializing) == 0x2f88);
static_assert(offsetof(DataSegment, unit_order_active) == 0x2f8a);
static_assert(offsetof(DataSegment, schedule_flags) == 0x2f8c);
static_assert(offsetof(DataSegment, turn_notification) == 0x2f8e);
static_assert(offsetof(DataSegment, turn_notification_bytes) == 0x2f8e);
static_assert(offsetof(DataSegment, overview_side) == 0x2f90);
static_assert(offsetof(DataSegment, view_side) == 0x2f91);
static_assert(offsetof(DataSegment, viewport_x) == 0x2f92);
static_assert(offsetof(DataSegment, viewport_x_bytes) == 0x2f92);
static_assert(offsetof(DataSegment, viewport_y) == 0x2f94);
static_assert(offsetof(DataSegment, viewport_y_bytes) == 0x2f94);
static_assert(offsetof(DataSegment, turn_frame_mode) == 0x2f96);
static_assert(offsetof(DataSegment, unknown_2f97) == 0x2f97);
static_assert(offsetof(DataSegment, control_selection) == 0x2f98);
static_assert(offsetof(DataSegment, context_display_word) == 0x2f9a);
static_assert(offsetof(DataSegment, context_side) == 0x2f9c);
static_assert(offsetof(DataSegment, unknown_2f9d) == 0x2f9d);
static_assert(offsetof(DataSegment, restart_flag) == 0x2f9e);
static_assert(offsetof(DataSegment, presentation_mode) == 0x2fa0);
static_assert(offsetof(DataSegment, battle_map_x) == 0x2fa2);
static_assert(offsetof(DataSegment, battle_x) == 0x2fa2);
static_assert(offsetof(DataSegment, battle_map_y) == 0x2fa3);
static_assert(offsetof(DataSegment, battle_y) == 0x2fa3);
static_assert(offsetof(DataSegment, battle_attacker_x) == 0x2fa4);
static_assert(offsetof(DataSegment, battle_attacker_y) == 0x2fa5);
static_assert(offsetof(DataSegment, battle_direction) == 0x2fa6);
static_assert(offsetof(DataSegment, battle_side) == 0x2fa7);
static_assert(offsetof(DataSegment, battle_command) == 0x2fa8);
static_assert(offsetof(DataSegment, battle_report) == 0x2fac);
static_assert(offsetof(DataSegment, battle_lists) == 0x2fae);
static_assert(offsetof(DataSegment, battle_units) == 0x2fae);
static_assert(offsetof(DataSegment, overview_objective_glyph) == 0x2fda);
static_assert(offsetof(DataSegment, flash_marker) == 0x2fe2);
static_assert(offsetof(DataSegment, face_offsets) == 0x2fea);
static_assert(offsetof(DataSegment, ai_unit_element_counts) == 0x3128);
static_assert(offsetof(DataSegment, request_capacities) == 0x3130);
static_assert(offsetof(DataSegment, capacity_a_weights) == 0x3140);
static_assert(offsetof(DataSegment, capacity_c_weights) == 0x3150);
static_assert(offsetof(DataSegment, person_templates) == 0x3160);
static_assert(offsetof(DataSegment, template_tables) == 0x32d4);
static_assert(offsetof(DataSegment, weather_obscurity) == 0x32d4);
static_assert(offsetof(DataSegment, air_cover) == 0x32da);
static_assert(offsetof(DataSegment, battle_levels) == 0x32f2);
static_assert(offsetof(DataSegment, text_3300) == 0x3300);
static_assert(offsetof(DataSegment, text_330e) == 0x330e);
static_assert(offsetof(DataSegment, text_331a) == 0x331a);
static_assert(offsetof(DataSegment, text_3326) == 0x3326);
static_assert(offsetof(DataSegment, unknown_3331) == 0x3331);
static_assert(offsetof(DataSegment, daily_records) == 0x3332);
static_assert(offsetof(DataSegment, element_types) == 0x3332);
static_assert(offsetof(DataSegment, weather_names) == 0x3b38);
static_assert(offsetof(DataSegment, ui_side_names) == 0x3b44);
static_assert(offsetof(DataSegment, side_names) == 0x3b44);
static_assert(offsetof(DataSegment, mask_names) == 0x3b48);
static_assert(offsetof(DataSegment, role_names) == 0x3b54);
static_assert(offsetof(DataSegment, entry_names) == 0x3b62);
static_assert(offsetof(DataSegment, cursor_code_text) == 0x3b72);
static_assert(offsetof(DataSegment, terrain_names) == 0x3b72);
static_assert(offsetof(DataSegment, order_names) == 0x3b88);
static_assert(offsetof(DataSegment, supply_names) == 0x3baa);
static_assert(offsetof(DataSegment, small_glyphs) == 0x3bb0);
static_assert(offsetof(DataSegment, battle_digits) == 0x3bb0);
static_assert(offsetof(DataSegment, digit_glyphs) == 0x3bb0);
static_assert(offsetof(DataSegment, rotated_glyphs) == 0x3bf2);
static_assert(offsetof(DataSegment, text_3c2a) == 0x3c2a);
static_assert(offsetof(DataSegment, text_3c2f) == 0x3c2f);
static_assert(offsetof(DataSegment, text_3c36) == 0x3c36);
static_assert(offsetof(DataSegment, text_3c3c) == 0x3c3c);
static_assert(offsetof(DataSegment, text_3c45) == 0x3c45);
static_assert(offsetof(DataSegment, text_3c4c) == 0x3c4c);
static_assert(offsetof(DataSegment, text_3c51) == 0x3c51);
static_assert(offsetof(DataSegment, text_3c56) == 0x3c56);
static_assert(offsetof(DataSegment, text_3c5c) == 0x3c5c);
static_assert(offsetof(DataSegment, text_3c63) == 0x3c63);
static_assert(offsetof(DataSegment, text_3c69) == 0x3c69);
static_assert(offsetof(DataSegment, text_3c6f) == 0x3c6f);
static_assert(offsetof(DataSegment, text_3c75) == 0x3c75);
static_assert(offsetof(DataSegment, text_3c7c) == 0x3c7c);
static_assert(offsetof(DataSegment, text_3c82) == 0x3c82);
static_assert(offsetof(DataSegment, text_3c89) == 0x3c89);
static_assert(offsetof(DataSegment, text_3c92) == 0x3c92);
static_assert(offsetof(DataSegment, battle_ai_side) == 0x3c99);
static_assert(offsetof(DataSegment, battle_active_side) == 0x3c9a);
static_assert(offsetof(DataSegment, battle_active_group) == 0x3c9b);
static_assert(offsetof(DataSegment, battle_ai_index) == 0x3c9c);
static_assert(offsetof(DataSegment, deploy_row) == 0x3c9d);
static_assert(offsetof(DataSegment, battle_hq_intact) == 0x3c9e);
static_assert(offsetof(DataSegment, battle_panel_shown) == 0x3c9e);
static_assert(offsetof(DataSegment, battle_cells) == 0x3ca2);
static_assert(offsetof(DataSegment, battle_numbers_shown) == 0x3d66);
static_assert(offsetof(DataSegment, unknown_3d67) == 0x3d67);
static_assert(offsetof(DataSegment, battle_ai_plans) == 0x3d68);
static_assert(offsetof(DataSegment, battle_ai_targets) == 0x3d7a);
static_assert(offsetof(DataSegment, battle_level_limit) == 0x3d9e);
static_assert(offsetof(DataSegment, unknown_3d9f) == 0x3d9f);
static_assert(offsetof(DataSegment, battle_selected) == 0x3da0);
static_assert(offsetof(DataSegment, battle_element_destroyed) == 0x3da2);
static_assert(offsetof(DataSegment, battle_element_destroyed_bytes) == 0x3da2);
static_assert(offsetof(DataSegment, battle_lost_element) == 0x3da4);
static_assert(offsetof(DataSegment, battle_lost_unit) == 0x3da6);
static_assert(offsetof(DataSegment, battle_ai_entry_side) == 0x3da8);
static_assert(offsetof(DataSegment, battle_exit_direction) == 0x3da8);
static_assert(offsetof(DataSegment, battle_side_routed) == 0x3daa);
static_assert(offsetof(DataSegment, battle_type_factors) == 0x3dae);
static_assert(offsetof(DataSegment, battle_spot_range) == 0x3db6);
static_assert(offsetof(DataSegment, battle_terrain_extras) == 0x3dbe);
static_assert(offsetof(DataSegment, battle_order_names) == 0x3ddc);
static_assert(offsetof(DataSegment, battle_terrain_names) == 0x3de8);
static_assert(offsetof(DataSegment, battle_terrain_costs) == 0x3dfe);
static_assert(offsetof(DataSegment, battle_terrain_cover) == 0x3e56);
static_assert(offsetof(DataSegment, battle_visibility) == 0x3e57);
static_assert(offsetof(DataSegment, unknown_3e58) == 0x3e58);
static_assert(offsetof(DataSegment, fileio_disk_abort_message) == 0x3e6e);
static_assert(offsetof(DataSegment, vgalib_data) == 0x3e9a);
static_assert(offsetof(DataSegment, unknown_3e9a) == 0x3e9a);
static_assert(offsetof(DataSegment, vgalib_line_error) == 0x3eaa);
static_assert(offsetof(DataSegment, vgalib_pattern_bit) == 0x3eac);
static_assert(offsetof(DataSegment, vgalib_pattern_on) == 0x3ead);
static_assert(offsetof(DataSegment, vgalib_var_3eb4) == 0x3eb4);
static_assert(offsetof(DataSegment, vgalib_decode_flag) == 0x3eb6);
static_assert(offsetof(DataSegment, vgalib_decode_handler) == 0x3eb8);
static_assert(offsetof(DataSegment, text_vars) == 0x3eba);
static_assert(offsetof(DataSegment, idle_hook) == 0x3eba);
static_assert(offsetof(DataSegment, unknown_3ebe) == 0x3ebe);
static_assert(offsetof(DataSegment, message_hook) == 0x3ec2);
static_assert(offsetof(DataSegment, text_x) == 0x3eca);
static_assert(offsetof(DataSegment, text_y) == 0x3ecc);
static_assert(offsetof(DataSegment, text_left) == 0x3ece);
static_assert(offsetof(DataSegment, text_right) == 0x3ed0);
static_assert(offsetof(DataSegment, text_top) == 0x3ed2);
static_assert(offsetof(DataSegment, text_bottom) == 0x3ed4);
static_assert(offsetof(DataSegment, text_height) == 0x3ed6);
static_assert(offsetof(DataSegment, text_scroll) == 0x3ed8);
static_assert(offsetof(DataSegment, text_paper) == 0x3eda);
static_assert(offsetof(DataSegment, text_ink) == 0x3edb);
static_assert(offsetof(DataSegment, text_background) == 0x3edc);
static_assert(offsetof(DataSegment, unknown_3edd) == 0x3edd);
static_assert(offsetof(DataSegment, saved_x) == 0x3ede);
static_assert(offsetof(DataSegment, saved_y) == 0x3ee0);
static_assert(offsetof(DataSegment, saved_left) == 0x3ee2);
static_assert(offsetof(DataSegment, saved_paper) == 0x3ee4);
static_assert(offsetof(DataSegment, saved_ink) == 0x3ee5);
static_assert(offsetof(DataSegment, saved_background) == 0x3ee6);
static_assert(offsetof(DataSegment, text_lead) == 0x3ee7);
static_assert(offsetof(DataSegment, text_state) == 0x3ee8);
static_assert(offsetof(DataSegment, format_output) == 0x3eea);
static_assert(offsetof(DataSegment, text_font) == 0x3eec);
static_assert(offsetof(DataSegment, setup_text) == 0x44ec);
static_assert(offsetof(DataSegment, glyph_draw) == 0x44ec);
static_assert(offsetof(DataSegment, text_erase) == 0x44f4);
static_assert(offsetof(DataSegment, prompt_format) == 0x44fc);
static_assert(offsetof(DataSegment, question_format) == 0x44ff);
static_assert(offsetof(DataSegment, input_mode) == 0x4502);
static_assert(offsetof(DataSegment, input_mode_bytes) == 0x4502);
static_assert(offsetof(DataSegment, menu_format) == 0x4504);
static_assert(offsetof(DataSegment, unknown_4511) == 0x4511);
static_assert(offsetof(DataSegment, menu_save_offset) == 0x4512);
static_assert(offsetof(DataSegment, numdlg_x) == 0x4514);
static_assert(offsetof(DataSegment, numdlg_y) == 0x4516);
static_assert(offsetof(DataSegment, numdlg_digits) == 0x4518);
static_assert(offsetof(DataSegment, numdlg_format) == 0x451d);
static_assert(offsetof(DataSegment, unknown_4527) == 0x4527);
static_assert(offsetof(DataSegment, ems_device_name) == 0x4528);
static_assert(offsetof(DataSegment, numdlg_image_mapped) == 0x4530);
static_assert(offsetof(DataSegment, tile_mapped) == 0x4532);
static_assert(offsetof(DataSegment, sprite_mapped) == 0x4534);
static_assert(offsetof(DataSegment, frame_mapped) == 0x4536);
static_assert(offsetof(DataSegment, setup_marker_points) == 0x4538);
static_assert(offsetof(DataSegment, mgamen_blocks) == 0x4554);
static_assert(offsetof(DataSegment, savedata_name) == 0x4560);
static_assert(offsetof(DataSegment, no_saved_games_text) == 0x4569);
static_assert(offsetof(DataSegment, load_print_format) == 0x4583);
static_assert(offsetof(DataSegment, load_savedata_name) == 0x4586);
static_assert(offsetof(DataSegment, load_which_text) == 0x458f);
static_assert(offsetof(DataSegment, loading_text) == 0x45a0);
static_assert(offsetof(DataSegment, loaded_text) == 0x45b6);
static_assert(offsetof(DataSegment, load_failed_text) == 0x45d2);
static_assert(offsetof(DataSegment, text_45ef) == 0x45ef);
static_assert(offsetof(DataSegment, text_45f7) == 0x45f7);
static_assert(offsetof(DataSegment, shown_portrait) == 0x4600);
static_assert(offsetof(DataSegment, shown_portrait_bytes) == 0x4600);
static_assert(offsetof(DataSegment, play_dialog) == 0x4602);
static_assert(offsetof(DataSegment, bytes_4615) == 0x4615);
static_assert(offsetof(DataSegment, reinforce_dialog) == 0x4616);
static_assert(offsetof(DataSegment, division_reserve_mark) == 0x4629);
static_assert(offsetof(DataSegment, empty_suffix) == 0x4633);
static_assert(offsetof(DataSegment, division_strength_format) == 0x4634);
static_assert(offsetof(DataSegment, chief_ops_format) == 0x4653);
static_assert(offsetof(DataSegment, chief_ops_prompt) == 0x4663);
static_assert(offsetof(DataSegment, chief_intel_format) == 0x4689);
static_assert(offsetof(DataSegment, chief_intel_prompt) == 0x4699);
static_assert(offsetof(DataSegment, division_format) == 0x46c0);
static_assert(offsetof(DataSegment, division_short_format) == 0x46d0);
static_assert(offsetof(DataSegment, division_long_format) == 0x46de);
static_assert(offsetof(DataSegment, role_prompt) == 0x46ee);
static_assert(offsetof(DataSegment, division_prompt) == 0x46fe);
static_assert(offsetof(DataSegment, general_header) == 0x471c);
static_assert(offsetof(DataSegment, general_name_format) == 0x4745);
static_assert(offsetof(DataSegment, general_stats_format) == 0x474e);
static_assert(offsetof(DataSegment, text_477c) == 0x477c);
static_assert(offsetof(DataSegment, text_478a) == 0x478a);
static_assert(offsetof(DataSegment, side_title_format) == 0x479a);
static_assert(offsetof(DataSegment, unknown_47a3) == 0x47a3);
static_assert(offsetof(DataSegment, side_titles) == 0x47a4);
static_assert(offsetof(DataSegment, reinforce_title) == 0x47a8);
static_assert(offsetof(DataSegment, reinforce_mouse_help) == 0x47bf);
static_assert(offsetof(DataSegment, reinforce_key_help) == 0x4819);
static_assert(offsetof(DataSegment, play_prompt) == 0x4870);
static_assert(offsetof(DataSegment, use_generals_prompt) == 0x4884);
static_assert(offsetof(DataSegment, scenario_message_format) == 0x4898);
static_assert(offsetof(DataSegment, unknown_48a8) == 0x48a8);
static_assert(offsetof(DataSegment, text_48aa) == 0x48aa);
static_assert(offsetof(DataSegment, text_48b4) == 0x48b4);
static_assert(offsetof(DataSegment, text_48c0) == 0x48c0);
static_assert(offsetof(DataSegment, text_48cb) == 0x48cb);
static_assert(offsetof(DataSegment, text_48d7) == 0x48d7);
static_assert(offsetof(DataSegment, text_48e4) == 0x48e4);
static_assert(offsetof(DataSegment, text_48f0) == 0x48f0);
static_assert(offsetof(DataSegment, text_48fa) == 0x48fa);
static_assert(offsetof(DataSegment, text_4905) == 0x4905);
static_assert(offsetof(DataSegment, text_4911) == 0x4911);
static_assert(offsetof(DataSegment, text_4920) == 0x4920);
static_assert(offsetof(DataSegment, text_492d) == 0x492d);
static_assert(offsetof(DataSegment, text_4933) == 0x4933);
static_assert(offsetof(DataSegment, text_493f) == 0x493f);
static_assert(offsetof(DataSegment, text_4952) == 0x4952);
static_assert(offsetof(DataSegment, text_495d) == 0x495d);
static_assert(offsetof(DataSegment, text_4963) == 0x4963);
static_assert(offsetof(DataSegment, text_496c) == 0x496c);
static_assert(offsetof(DataSegment, text_4972) == 0x4972);
static_assert(offsetof(DataSegment, text_497d) == 0x497d);
static_assert(offsetof(DataSegment, text_4991) == 0x4991);
static_assert(offsetof(DataSegment, text_49ae) == 0x49ae);
static_assert(offsetof(DataSegment, text_49cb) == 0x49cb);
static_assert(offsetof(DataSegment, text_49de) == 0x49de);
static_assert(offsetof(DataSegment, text_49f7) == 0x49f7);
static_assert(offsetof(DataSegment, scensel_colors) == 0x4a0e);
static_assert(offsetof(DataSegment, scensel_empty) == 0x4a18);
static_assert(offsetof(DataSegment, scensel_france) == 0x4a19);
static_assert(offsetof(DataSegment, scensel_africa) == 0x4a2e);
static_assert(offsetof(DataSegment, scensel_kursk) == 0x4a48);
static_assert(offsetof(DataSegment, scensel_dday) == 0x4a5a);
static_assert(offsetof(DataSegment, scensel_bulge) == 0x4a6b);
static_assert(offsetof(DataSegment, scensel_berlin) == 0x4a81);
static_assert(offsetof(DataSegment, scensel_demo) == 0x4a92);
static_assert(offsetof(DataSegment, scensel_view_format) == 0x4aa3);
static_assert(offsetof(DataSegment, scensel_view_line) == 0x4ab7);
static_assert(offsetof(DataSegment, scensel_one_player) == 0x4ac7);
static_assert(offsetof(DataSegment, scensel_command_format) == 0x4ae4);
static_assert(offsetof(DataSegment, scensel_command_line) == 0x4af8);
static_assert(offsetof(DataSegment, scensel_two_players) == 0x4b0b);
static_assert(offsetof(DataSegment, scensel_aerial) == 0x4b29);
static_assert(offsetof(DataSegment, scensel_field) == 0x4b31);
static_assert(offsetof(DataSegment, scensel_map_format) == 0x4b39);
static_assert(offsetof(DataSegment, scensel_map_line) == 0x4b4e);
static_assert(offsetof(DataSegment, scensel_view_all) == 0x4b61);
static_assert(offsetof(DataSegment, scensel_view_none) == 0x4b78);
static_assert(offsetof(DataSegment, scensel_view_selected) == 0x4b8f);
static_assert(offsetof(DataSegment, scensel_battles_line) == 0x4ba6);
static_assert(offsetof(DataSegment, scensel_difficulty) == 0x4bb9);
static_assert(offsetof(DataSegment, scensel_very_easy) == 0x4bcc);
static_assert(offsetof(DataSegment, scensel_easy) == 0x4bd6);
static_assert(offsetof(DataSegment, scensel_average) == 0x4be0);
static_assert(offsetof(DataSegment, scensel_hard) == 0x4bea);
static_assert(offsetof(DataSegment, scensel_very_hard) == 0x4bf4);
static_assert(offsetof(DataSegment, scensel_difficulty_line) == 0x4bfe);
static_assert(offsetof(DataSegment, scensel_settings_ok) == 0x4c11);
static_assert(offsetof(DataSegment, scensel_reselect) == 0x4c26);
static_assert(offsetof(DataSegment, scensel_ready_demo) == 0x4c48);
static_assert(offsetof(DataSegment, scensel_ready_colors) == 0x4c6b);
static_assert(offsetof(DataSegment, scensel_load_colors) == 0x4c72);
static_assert(offsetof(DataSegment, scensel_ready_blank) == 0x4c79);
static_assert(offsetof(DataSegment, scensel_ready) == 0x4c96);
static_assert(offsetof(DataSegment, scensel_done_colors) == 0x4cb1);
static_assert(offsetof(DataSegment, unknown_4cbb) == 0x4cbb);
static_assert(offsetof(DataSegment, scensel_start_menu) == 0x4cbc);
static_assert(offsetof(DataSegment, near_4cc4) == 0x4cc4);
static_assert(offsetof(DataSegment, words_4cc6) == 0x4cc6);
static_assert(offsetof(DataSegment, near_4cd0) == 0x4cd0);
static_assert(offsetof(DataSegment, bytes_4cd2) == 0x4cd2);
static_assert(offsetof(DataSegment, scensel_start_prompt) == 0x4ce0);
static_assert(offsetof(DataSegment, scensel_players_menu) == 0x4d00);
static_assert(offsetof(DataSegment, near_4d08) == 0x4d08);
static_assert(offsetof(DataSegment, words_4d0a) == 0x4d0a);
static_assert(offsetof(DataSegment, near_4d14) == 0x4d14);
static_assert(offsetof(DataSegment, words_4d16) == 0x4d16);
static_assert(offsetof(DataSegment, near_4d20) == 0x4d20);
static_assert(offsetof(DataSegment, bytes_4d22) == 0x4d22);
static_assert(offsetof(DataSegment, scensel_players_prompt) == 0x4d30);
static_assert(offsetof(DataSegment, scensel_view_prompt) == 0x4d48);
static_assert(offsetof(DataSegment, scensel_side_menu) == 0x4d68);
static_assert(offsetof(DataSegment, near_4d70) == 0x4d70);
static_assert(offsetof(DataSegment, words_4d72) == 0x4d72);
static_assert(offsetof(DataSegment, near_4d7c) == 0x4d7c);
static_assert(offsetof(DataSegment, bytes_4d7e) == 0x4d7e);
static_assert(offsetof(DataSegment, scensel_command_prompt) == 0x4d8c);
static_assert(offsetof(DataSegment, unknown_4daf) == 0x4daf);
static_assert(offsetof(DataSegment, scensel_map_menu) == 0x4db0);
static_assert(offsetof(DataSegment, near_4db8) == 0x4db8);
static_assert(offsetof(DataSegment, words_4dba) == 0x4dba);
static_assert(offsetof(DataSegment, near_4dc4) == 0x4dc4);
static_assert(offsetof(DataSegment, bytes_4dc6) == 0x4dc6);
static_assert(offsetof(DataSegment, scensel_map_prompt) == 0x4dd4);
static_assert(offsetof(DataSegment, scensel_battles_menu) == 0x4df2);
static_assert(offsetof(DataSegment, near_4dfa) == 0x4dfa);
static_assert(offsetof(DataSegment, words_4dfc) == 0x4dfc);
static_assert(offsetof(DataSegment, near_4e06) == 0x4e06);
static_assert(offsetof(DataSegment, bytes_4e08) == 0x4e08);
static_assert(offsetof(DataSegment, scensel_battles_prompt) == 0x4e16);
static_assert(offsetof(DataSegment, unknown_4e2b) == 0x4e2b);
static_assert(offsetof(DataSegment, scensel_battles_menu2) == 0x4e2c);
static_assert(offsetof(DataSegment, near_4e34) == 0x4e34);
static_assert(offsetof(DataSegment, words_4e36) == 0x4e36);
static_assert(offsetof(DataSegment, near_4e40) == 0x4e40);
static_assert(offsetof(DataSegment, words_4e42) == 0x4e42);
static_assert(offsetof(DataSegment, near_4e4c) == 0x4e4c);
static_assert(offsetof(DataSegment, bytes_4e4e) == 0x4e4e);
static_assert(offsetof(DataSegment, scensel_battles_prompt2) == 0x4e5c);
static_assert(offsetof(DataSegment, unknown_4e71) == 0x4e71);
static_assert(offsetof(DataSegment, scensel_difficulty_menu) == 0x4e72);
static_assert(offsetof(DataSegment, near_4e7a) == 0x4e7a);
static_assert(offsetof(DataSegment, words_4e7c) == 0x4e7c);
static_assert(offsetof(DataSegment, near_4e86) == 0x4e86);
static_assert(offsetof(DataSegment, words_4e88) == 0x4e88);
static_assert(offsetof(DataSegment, near_4e92) == 0x4e92);
static_assert(offsetof(DataSegment, words_4e94) == 0x4e94);
static_assert(offsetof(DataSegment, near_4e9e) == 0x4e9e);
static_assert(offsetof(DataSegment, bytes_4ea0) == 0x4ea0);
static_assert(offsetof(DataSegment, near_4eaa) == 0x4eaa);
static_assert(offsetof(DataSegment, bytes_4eac) == 0x4eac);
static_assert(offsetof(DataSegment, scensel_difficulty_prompt) == 0x4eba);
static_assert(offsetof(DataSegment, scensel_scenario_menu) == 0x4ed8);
static_assert(offsetof(DataSegment, near_4ee0) == 0x4ee0);
static_assert(offsetof(DataSegment, words_4ee2) == 0x4ee2);
static_assert(offsetof(DataSegment, near_4eec) == 0x4eec);
static_assert(offsetof(DataSegment, words_4eee) == 0x4eee);
static_assert(offsetof(DataSegment, near_4ef8) == 0x4ef8);
static_assert(offsetof(DataSegment, words_4efa) == 0x4efa);
static_assert(offsetof(DataSegment, near_4f04) == 0x4f04);
static_assert(offsetof(DataSegment, words_4f06) == 0x4f06);
static_assert(offsetof(DataSegment, near_4f10) == 0x4f10);
static_assert(offsetof(DataSegment, bytes_4f12) == 0x4f12);
static_assert(offsetof(DataSegment, near_4f1c) == 0x4f1c);
static_assert(offsetof(DataSegment, bytes_4f1e) == 0x4f1e);
static_assert(offsetof(DataSegment, scensel_scenario_prompt) == 0x4f2c);
static_assert(offsetof(DataSegment, scensel_pictures) == 0x4f50);
static_assert(offsetof(DataSegment, memory_message) == 0x4f5e);
static_assert(offsetof(DataSegment, memory_advice) == 0x4fa1);
static_assert(offsetof(DataSegment, memory_exit) == 0x4fe9);
static_assert(offsetof(DataSegment, setup_name_bmapseg) == 0x5005);
static_assert(offsetof(DataSegment, setup_name_hunitseg) == 0x500d);
static_assert(offsetof(DataSegment, setup_name_grpseg) == 0x5016);
static_assert(offsetof(DataSegment, setup_name_ms_winseg) == 0x501d);
static_assert(offsetof(DataSegment, setup_name_vseg) == 0x5027);
static_assert(offsetof(DataSegment, setup_name_hdispseg) == 0x502c);
static_assert(offsetof(DataSegment, setup_name_armsseg) == 0x5035);
static_assert(offsetof(DataSegment, setup_name_kaoseg) == 0x503d);
static_assert(offsetof(DataSegment, setup_name_mdispseg) == 0x5044);
static_assert(offsetof(DataSegment, setup_name_ms_seg) == 0x504d);
static_assert(offsetof(DataSegment, setup_name_hfontseg) == 0x5054);
static_assert(offsetof(DataSegment, setup_name_mmapseg) == 0x505d);
static_assert(offsetof(DataSegment, unknown_5065) == 0x5065);
static_assert(offsetof(DataSegment, end_palette) == 0x5066);
static_assert(offsetof(DataSegment, end_palette_final) == 0x5096);
static_assert(offsetof(DataSegment, text_50c1) == 0x50c1);
static_assert(offsetof(DataSegment, end_text_format) == 0x50c6);
static_assert(offsetof(DataSegment, end_picture_name) == 0x50d2);
static_assert(offsetof(DataSegment, end_salute) == 0x50dd);
static_assert(offsetof(DataSegment, end_victory_texts) == 0x50f8);
static_assert(offsetof(DataSegment, end_defeat_texts) == 0x5108);
static_assert(offsetof(DataSegment, name_bmapseg) == 0x5118);
static_assert(offsetof(DataSegment, name_mmapseg) == 0x5120);
static_assert(offsetof(DataSegment, name_hdispseg) == 0x5128);
static_assert(offsetof(DataSegment, name_hunitseg) == 0x5131);
static_assert(offsetof(DataSegment, name_grpseg) == 0x513a);
static_assert(offsetof(DataSegment, name_ms_winseg) == 0x5141);
static_assert(offsetof(DataSegment, name_ms_seg) == 0x514b);
static_assert(offsetof(DataSegment, name_hfontseg) == 0x5152);
static_assert(offsetof(DataSegment, name_vseg) == 0x515b);
static_assert(offsetof(DataSegment, name_kaoseg) == 0x5160);
static_assert(offsetof(DataSegment, name_armsseg) == 0x5167);
static_assert(offsetof(DataSegment, name_mdispseg) == 0x516f);
static_assert(offsetof(DataSegment, calendar_days) == 0x5177);
static_assert(offsetof(DataSegment, startup_text) == 0x5184);
static_assert(offsetof(DataSegment, startup_mode) == 0x5184);
static_assert(offsetof(DataSegment, input_prompt) == 0x5185);
static_assert(offsetof(DataSegment, stream_count) == 0x51c0);
static_assert(offsetof(DataSegment, portrait_nodes) == 0x51c2);
static_assert(offsetof(DataSegment, portrait_head) == 0x51d6);
static_assert(offsetof(DataSegment, face_nodes) == 0x51d8);
static_assert(offsetof(DataSegment, face_head) == 0x51ec);
static_assert(offsetof(DataSegment, backdrop_pattern) == 0x51ee);
static_assert(offsetof(DataSegment, backdrop_color_pattern) == 0x520e);
static_assert(offsetof(DataSegment, palette_base) == 0x522e);
static_assert(offsetof(DataSegment, palette_dawn) == 0x5246);
static_assert(offsetof(DataSegment, palette_day) == 0x525e);
static_assert(offsetof(DataSegment, palette_dusk) == 0x5276);
static_assert(offsetof(DataSegment, palette_night) == 0x528e);
static_assert(offsetof(DataSegment, palette_title) == 0x52a6);
static_assert(offsetof(DataSegment, palette_map) == 0x52d6);
static_assert(offsetof(DataSegment, text_5301) == 0x5301);
static_assert(offsetof(DataSegment, general_stats_line) == 0x5306);
static_assert(offsetof(DataSegment, div_commander_prompt) == 0x5336);
static_assert(offsetof(DataSegment, step_data) == 0x536c);
static_assert(offsetof(DataSegment, step_biased_dx) == 0x536c);
static_assert(offsetof(DataSegment, step_biased_dy) == 0x5374);
static_assert(offsetof(DataSegment, label_text) == 0x537c);
static_assert(offsetof(DataSegment, unit_name_format) == 0x537c);
static_assert(offsetof(DataSegment, type_name_suffix) == 0x5383);
static_assert(offsetof(DataSegment, item_name_suffix) == 0x5389);
static_assert(offsetof(DataSegment, location_name_format) == 0x538a);
static_assert(offsetof(DataSegment, deputy_identity_ranges) == 0x5392);
static_assert(offsetof(DataSegment, deputy_name) == 0x5396);
static_assert(offsetof(DataSegment, army_out) == 0x53a2);
static_assert(offsetof(DataSegment, army_row0) == 0x53bd);
static_assert(offsetof(DataSegment, army_row1) == 0x53f1);
static_assert(offsetof(DataSegment, army_row2) == 0x5422);
static_assert(offsetof(DataSegment, army_row3) == 0x5453);
static_assert(offsetof(DataSegment, army_row4) == 0x5474);
static_assert(offsetof(DataSegment, army_row5) == 0x549b);
static_assert(offsetof(DataSegment, army_hq) == 0x54c0);
static_assert(offsetof(DataSegment, army_person_format) == 0x54d4);
static_assert(offsetof(DataSegment, army_hq_label) == 0x54e0);
static_assert(offsetof(DataSegment, division_label_format) == 0x54e8);
static_assert(offsetof(DataSegment, division_person_format) == 0x54f7);
static_assert(offsetof(DataSegment, panel_name_format) == 0x5503);
static_assert(offsetof(DataSegment, panel_count_format) == 0x5509);
static_assert(offsetof(DataSegment, text_5514) == 0x5514);
static_assert(offsetof(DataSegment, text_551c) == 0x551c);
static_assert(offsetof(DataSegment, text_5524) == 0x5524);
static_assert(offsetof(DataSegment, text_552d) == 0x552d);
static_assert(offsetof(DataSegment, panel_regiment_format) == 0x5534);
static_assert(offsetof(DataSegment, panel_battle) == 0x5545);
static_assert(offsetof(DataSegment, panel_string_format) == 0x554c);
static_assert(offsetof(DataSegment, unknown_554f) == 0x554f);
static_assert(offsetof(DataSegment, panel_regiment_names) == 0x5550);
static_assert(offsetof(DataSegment, credits_title) == 0x5558);
static_assert(offsetof(DataSegment, credits_main) == 0x557c);
static_assert(offsetof(DataSegment, credits_support) == 0x5598);
static_assert(offsetof(DataSegment, credits_translation) == 0x55b9);
static_assert(offsetof(DataSegment, unknown_55f3) == 0x55f3);
static_assert(offsetof(DataSegment, tbl_group_header) == 0x55f4);
static_assert(offsetof(DataSegment, tbl_group_title) == 0x561a);
static_assert(offsetof(DataSegment, tbl_group_mark) == 0x561d);
static_assert(offsetof(DataSegment, tbl_group_division) == 0x5621);
static_assert(offsetof(DataSegment, tbl_group_empty) == 0x5633);
static_assert(offsetof(DataSegment, tbl_group_div) == 0x563d);
static_assert(offsetof(DataSegment, tbl_group_army) == 0x5643);
static_assert(offsetof(DataSegment, tbl_group_row) == 0x564a);
static_assert(offsetof(DataSegment, tbl_group_blank) == 0x5670);
static_assert(offsetof(DataSegment, text_5697) == 0x5697);
static_assert(offsetof(DataSegment, tbl_mission_header) == 0x569e);
static_assert(offsetof(DataSegment, tbl_mission_label) == 0x56a8);
static_assert(offsetof(DataSegment, tbl_mission_place) == 0x56b6);
static_assert(offsetof(DataSegment, tbl_mission_target) == 0x56c7);
static_assert(offsetof(DataSegment, tbl_mission_row) == 0x56d1);
static_assert(offsetof(DataSegment, tbl_mission_blank) == 0x56d9);
static_assert(offsetof(DataSegment, unknown_56ff) == 0x56ff);
static_assert(offsetof(DataSegment, division_dialog) == 0x5700);
static_assert(offsetof(DataSegment, bytes_5713) == 0x5713);
static_assert(offsetof(DataSegment, tbl_unit_header) == 0x5716);
static_assert(offsetof(DataSegment, tbl_unit_mark) == 0x5745);
static_assert(offsetof(DataSegment, tbl_unit_label) == 0x5749);
static_assert(offsetof(DataSegment, tbl_unit_empty) == 0x5754);
static_assert(offsetof(DataSegment, tbl_unit_blank) == 0x5761);
static_assert(offsetof(DataSegment, tbl_unit_wait) == 0x5791);
static_assert(offsetof(DataSegment, tbl_unit_row) == 0x5796);
static_assert(offsetof(DataSegment, tbl_unit_hidden) == 0x57c0);
static_assert(offsetof(DataSegment, unknown_57ed) == 0x57ed);
static_assert(offsetof(DataSegment, order_target_list) == 0x57ee);
static_assert(offsetof(DataSegment, bytes_5801) == 0x5801);
static_assert(offsetof(DataSegment, tbl_order_header) == 0x5809);
static_assert(offsetof(DataSegment, tbl_order_label) == 0x583c);
static_assert(offsetof(DataSegment, unknown_5847) == 0x5847);
static_assert(offsetof(DataSegment, organize_list) == 0x5848);
static_assert(offsetof(DataSegment, bytes_585b) == 0x585b);
static_assert(offsetof(DataSegment, tbl_element_header) == 0x5863);
static_assert(offsetof(DataSegment, tbl_element_mark) == 0x5881);
static_assert(offsetof(DataSegment, tbl_element_name) == 0x5885);
static_assert(offsetof(DataSegment, tbl_element_row) == 0x588f);
static_assert(offsetof(DataSegment, regiment_dialog) == 0x58ae);
static_assert(offsetof(DataSegment, bytes_58c1) == 0x58c1);
static_assert(offsetof(DataSegment, text_58c8) == 0x58c8);
static_assert(offsetof(DataSegment, text_58cd) == 0x58cd);
static_assert(offsetof(DataSegment, text_58d2) == 0x58d2);
static_assert(offsetof(DataSegment, text_58d7) == 0x58d7);
static_assert(offsetof(DataSegment, text_58dc) == 0x58dc);
static_assert(offsetof(DataSegment, text_58e2) == 0x58e2);
static_assert(offsetof(DataSegment, text_58e8) == 0x58e8);
static_assert(offsetof(DataSegment, tbl_person_name) == 0x58ee);
static_assert(offsetof(DataSegment, tbl_person_blank) == 0x58f7);
static_assert(offsetof(DataSegment, tbl_person_rest) == 0x58fc);
static_assert(offsetof(DataSegment, tbl_person_hurt) == 0x5901);
static_assert(offsetof(DataSegment, choice_dialog) == 0x5906);
static_assert(offsetof(DataSegment, bytes_5919) == 0x5919);
static_assert(offsetof(DataSegment, tbl_post_names) == 0x591a);
static_assert(offsetof(DataSegment, bytes_5928) == 0x5928);
static_assert(offsetof(DataSegment, tbl_type_header) == 0x5935);
static_assert(offsetof(DataSegment, tbl_type_name) == 0x595c);
static_assert(offsetof(DataSegment, tbl_type_row) == 0x5965);
static_assert(offsetof(DataSegment, people_dialog) == 0x598a);
static_assert(offsetof(DataSegment, text_599d) == 0x599d);
static_assert(offsetof(DataSegment, tbl_supply_header) == 0x59a5);
static_assert(offsetof(DataSegment, tbl_supply_name) == 0x59d5);
static_assert(offsetof(DataSegment, tbl_supply_row) == 0x59de);
static_assert(offsetof(DataSegment, tbl_supply_blank) == 0x5a0d);
static_assert(offsetof(DataSegment, unknown_5a3d) == 0x5a3d);
static_assert(offsetof(DataSegment, weapons_dialog) == 0x5a3e);
static_assert(offsetof(DataSegment, bytes_5a51) == 0x5a51);
static_assert(offsetof(DataSegment, tbl_str_header) == 0x5a57);
static_assert(offsetof(DataSegment, tbl_str_name) == 0x5a64);
static_assert(offsetof(DataSegment, tbl_str_row) == 0x5a6c);
static_assert(offsetof(DataSegment, unknown_5a77) == 0x5a77);
static_assert(offsetof(DataSegment, city_dialog) == 0x5a78);
static_assert(offsetof(DataSegment, bytes_5a8b) == 0x5a8b);
static_assert(offsetof(DataSegment, stock_dialog) == 0x5a96);
static_assert(offsetof(DataSegment, unknown_5aa9) == 0x5aa9);
static_assert(offsetof(DataSegment, save_menu) == 0x5aaa);
static_assert(offsetof(DataSegment, far_code_5ab5) == 0x5ab5);
static_assert(offsetof(DataSegment, far_code_5ab9) == 0x5ab9);
static_assert(offsetof(DataSegment, text_5abd) == 0x5abd);
static_assert(offsetof(DataSegment, text_5ac6) == 0x5ac6);
static_assert(offsetof(DataSegment, tbl_save_header) == 0x5acf);
static_assert(offsetof(DataSegment, tbl_save_number) == 0x5b05);
static_assert(offsetof(DataSegment, tbl_map_france) == 0x5b0c);
static_assert(offsetof(DataSegment, tbl_map_africa) == 0x5b13);
static_assert(offsetof(DataSegment, tbl_map_kursk) == 0x5b1a);
static_assert(offsetof(DataSegment, tbl_map_dday) == 0x5b21);
static_assert(offsetof(DataSegment, tbl_map_bulge) == 0x5b28);
static_assert(offsetof(DataSegment, tbl_map_berlin) == 0x5b2f);
static_assert(offsetof(DataSegment, tbl_level_veasy) == 0x5b36);
static_assert(offsetof(DataSegment, tbl_level_easy) == 0x5b3d);
static_assert(offsetof(DataSegment, tbl_level_avg) == 0x5b44);
static_assert(offsetof(DataSegment, tbl_level_hard) == 0x5b4b);
static_assert(offsetof(DataSegment, tbl_level_vhard) == 0x5b52);
static_assert(offsetof(DataSegment, tbl_save_row) == 0x5b59);
static_assert(offsetof(DataSegment, unknown_5b83) == 0x5b83);
static_assert(offsetof(DataSegment, tbl_player_names) == 0x5b84);
static_assert(offsetof(DataSegment, music_current) == 0x5b88);
static_assert(offsetof(DataSegment, effect_current) == 0x5b89);
static_assert(offsetof(DataSegment, music_bank) == 0x5b8a);
static_assert(offsetof(DataSegment, music_path) == 0x5ba4);
static_assert(offsetof(DataSegment, briefing_text) == 0x5bb2);
static_assert(offsetof(DataSegment, text_5bb2) == 0x5bb2);
static_assert(offsetof(DataSegment, text_5c13) == 0x5c13);
static_assert(offsetof(DataSegment, text_5c4c) == 0x5c4c);
static_assert(offsetof(DataSegment, text_5c84) == 0x5c84);
static_assert(offsetof(DataSegment, text_5cab) == 0x5cab);
static_assert(offsetof(DataSegment, text_5cde) == 0x5cde);
static_assert(offsetof(DataSegment, text_5d1c) == 0x5d1c);
static_assert(offsetof(DataSegment, text_5d5e) == 0x5d5e);
static_assert(offsetof(DataSegment, text_5da4) == 0x5da4);
static_assert(offsetof(DataSegment, text_5dde) == 0x5dde);
static_assert(offsetof(DataSegment, text_5e15) == 0x5e15);
static_assert(offsetof(DataSegment, text_5e56) == 0x5e56);
static_assert(offsetof(DataSegment, text_5e83) == 0x5e83);
static_assert(offsetof(DataSegment, text_5ea9) == 0x5ea9);
static_assert(offsetof(DataSegment, text_5edb) == 0x5edb);
static_assert(offsetof(DataSegment, text_5f01) == 0x5f01);
static_assert(offsetof(DataSegment, text_5f36) == 0x5f36);
static_assert(offsetof(DataSegment, text_5f5c) == 0x5f5c);
static_assert(offsetof(DataSegment, text_5f81) == 0x5f81);
static_assert(offsetof(DataSegment, text_5fb0) == 0x5fb0);
static_assert(offsetof(DataSegment, text_5fdb) == 0x5fdb);
static_assert(offsetof(DataSegment, text_6008) == 0x6008);
static_assert(offsetof(DataSegment, text_603e) == 0x603e);
static_assert(offsetof(DataSegment, text_6070) == 0x6070);
static_assert(offsetof(DataSegment, text_60b5) == 0x60b5);
static_assert(offsetof(DataSegment, text_60e2) == 0x60e2);
static_assert(offsetof(DataSegment, text_6115) == 0x6115);
static_assert(offsetof(DataSegment, text_6149) == 0x6149);
static_assert(offsetof(DataSegment, text_617f) == 0x617f);
static_assert(offsetof(DataSegment, text_61b3) == 0x61b3);
static_assert(offsetof(DataSegment, text_61d6) == 0x61d6);
static_assert(offsetof(DataSegment, text_621c) == 0x621c);
static_assert(offsetof(DataSegment, text_6263) == 0x6263);
static_assert(offsetof(DataSegment, text_62a3) == 0x62a3);
static_assert(offsetof(DataSegment, text_62d5) == 0x62d5);
static_assert(offsetof(DataSegment, text_6300) == 0x6300);
static_assert(offsetof(DataSegment, text_6330) == 0x6330);
static_assert(offsetof(DataSegment, text_636d) == 0x636d);
static_assert(offsetof(DataSegment, text_63ad) == 0x63ad);
static_assert(offsetof(DataSegment, text_63e7) == 0x63e7);
static_assert(offsetof(DataSegment, text_6420) == 0x6420);
static_assert(offsetof(DataSegment, text_6454) == 0x6454);
static_assert(offsetof(DataSegment, text_6492) == 0x6492);
static_assert(offsetof(DataSegment, text_64cd) == 0x64cd);
static_assert(offsetof(DataSegment, text_6507) == 0x6507);
static_assert(offsetof(DataSegment, text_6534) == 0x6534);
static_assert(offsetof(DataSegment, text_656b) == 0x656b);
static_assert(offsetof(DataSegment, text_659d) == 0x659d);
static_assert(offsetof(DataSegment, text_65d7) == 0x65d7);
static_assert(offsetof(DataSegment, text_6612) == 0x6612);
static_assert(offsetof(DataSegment, text_6639) == 0x6639);
static_assert(offsetof(DataSegment, text_6658) == 0x6658);
static_assert(offsetof(DataSegment, text_668c) == 0x668c);
static_assert(offsetof(DataSegment, text_66cb) == 0x66cb);
static_assert(offsetof(DataSegment, text_6708) == 0x6708);
static_assert(offsetof(DataSegment, text_674a) == 0x674a);
static_assert(offsetof(DataSegment, text_6780) == 0x6780);
static_assert(offsetof(DataSegment, text_67b7) == 0x67b7);
static_assert(offsetof(DataSegment, text_67e8) == 0x67e8);
static_assert(offsetof(DataSegment, text_682a) == 0x682a);
static_assert(offsetof(DataSegment, text_686b) == 0x686b);
static_assert(offsetof(DataSegment, text_68b6) == 0x68b6);
static_assert(offsetof(DataSegment, text_68cd) == 0x68cd);
static_assert(offsetof(DataSegment, text_68ff) == 0x68ff);
static_assert(offsetof(DataSegment, text_693a) == 0x693a);
static_assert(offsetof(DataSegment, text_6982) == 0x6982);
static_assert(offsetof(DataSegment, text_69db) == 0x69db);
static_assert(offsetof(DataSegment, text_6a14) == 0x6a14);
static_assert(offsetof(DataSegment, text_6a5c) == 0x6a5c);
static_assert(offsetof(DataSegment, text_6a91) == 0x6a91);
static_assert(offsetof(DataSegment, text_6ad5) == 0x6ad5);
static_assert(offsetof(DataSegment, text_6b1a) == 0x6b1a);
static_assert(offsetof(DataSegment, text_6b52) == 0x6b52);
static_assert(offsetof(DataSegment, text_6b9f) == 0x6b9f);
static_assert(offsetof(DataSegment, text_6be5) == 0x6be5);
static_assert(offsetof(DataSegment, text_6c17) == 0x6c17);
static_assert(offsetof(DataSegment, text_6c50) == 0x6c50);
static_assert(offsetof(DataSegment, text_6cab) == 0x6cab);
static_assert(offsetof(DataSegment, text_6ce6) == 0x6ce6);
static_assert(offsetof(DataSegment, text_6d30) == 0x6d30);
static_assert(offsetof(DataSegment, text_6d52) == 0x6d52);
static_assert(offsetof(DataSegment, text_6d8f) == 0x6d8f);
static_assert(offsetof(DataSegment, text_6db6) == 0x6db6);
static_assert(offsetof(DataSegment, text_6def) == 0x6def);
static_assert(offsetof(DataSegment, text_6e31) == 0x6e31);
static_assert(offsetof(DataSegment, text_6e76) == 0x6e76);
static_assert(offsetof(DataSegment, text_6ea7) == 0x6ea7);
static_assert(offsetof(DataSegment, text_6ed5) == 0x6ed5);
static_assert(offsetof(DataSegment, text_6ef1) == 0x6ef1);
static_assert(offsetof(DataSegment, text_6f1f) == 0x6f1f);
static_assert(offsetof(DataSegment, text_6f60) == 0x6f60);
static_assert(offsetof(DataSegment, text_6f99) == 0x6f99);
static_assert(offsetof(DataSegment, text_6fc2) == 0x6fc2);
static_assert(offsetof(DataSegment, text_7008) == 0x7008);
static_assert(offsetof(DataSegment, text_703f) == 0x703f);
static_assert(offsetof(DataSegment, text_707e) == 0x707e);
static_assert(offsetof(DataSegment, text_709d) == 0x709d);
static_assert(offsetof(DataSegment, text_70d8) == 0x70d8);
static_assert(offsetof(DataSegment, text_7113) == 0x7113);
static_assert(offsetof(DataSegment, text_7140) == 0x7140);
static_assert(offsetof(DataSegment, text_716c) == 0x716c);
static_assert(offsetof(DataSegment, text_719d) == 0x719d);
static_assert(offsetof(DataSegment, text_71e2) == 0x71e2);
static_assert(offsetof(DataSegment, text_7217) == 0x7217);
static_assert(offsetof(DataSegment, text_7235) == 0x7235);
static_assert(offsetof(DataSegment, text_726e) == 0x726e);
static_assert(offsetof(DataSegment, text_728e) == 0x728e);
static_assert(offsetof(DataSegment, text_72bc) == 0x72bc);
static_assert(offsetof(DataSegment, text_730e) == 0x730e);
static_assert(offsetof(DataSegment, side_initializers) == 0x735a);
static_assert(offsetof(DataSegment, event_tables) == 0x7372);
static_assert(offsetof(DataSegment, new_weapon_tank) == 0x7372);
static_assert(offsetof(DataSegment, new_weapon_period) == 0x7379);
static_assert(offsetof(DataSegment, new_weapon_format) == 0x737b);
static_assert(offsetof(DataSegment, unknown_73ad) == 0x73ad);
static_assert(offsetof(DataSegment, noon_role_style) == 0x73ae);
static_assert(offsetof(DataSegment, noon_name_format) == 0x73b5);
static_assert(offsetof(DataSegment, noon_speech_style) == 0x73ba);
static_assert(offsetof(DataSegment, noon_speech_end) == 0x73c4);
static_assert(offsetof(DataSegment, unknown_73cb) == 0x73cb);
static_assert(offsetof(DataSegment, noon_speeches) == 0x73cc);
static_assert(offsetof(DataSegment, noon_news_end) == 0x7490);
static_assert(offsetof(DataSegment, noon_events) == 0x7494);
static_assert(offsetof(DataSegment, mission_enemy_text) == 0x79b0);
static_assert(offsetof(DataSegment, mission_empty_text) == 0x79b7);
static_assert(offsetof(DataSegment, raid_attempt_text) == 0x79b8);
static_assert(offsetof(DataSegment, raid_attempt_tail) == 0x79cf);
static_assert(offsetof(DataSegment, raid_unit_text) == 0x79dc);
static_assert(offsetof(DataSegment, raid_unit_tail) == 0x79eb);
static_assert(offsetof(DataSegment, sabotage_text) == 0x79f8);
static_assert(offsetof(DataSegment, sabotage_tail) == 0x7a13);
static_assert(offsetof(DataSegment, mission3_suffix) == 0x7a23);
static_assert(offsetof(DataSegment, mission3_tail) == 0x7a25);
static_assert(offsetof(DataSegment, mission_bold_off) == 0x7a27);
static_assert(offsetof(DataSegment, recon_objective_text) == 0x7a2a);
static_assert(offsetof(DataSegment, recon_newline) == 0x7a42);
static_assert(offsetof(DataSegment, recon_unit_text) == 0x7a44);
static_assert(offsetof(DataSegment, mission_handlers) == 0x7a54);
static_assert(offsetof(DataSegment, month_climate) == 0x7a73);
static_assert(offsetof(DataSegment, air_postponed_text) == 0x7a80);
static_assert(offsetof(DataSegment, air_canceled_text) == 0x7aba);
static_assert(offsetof(DataSegment, air_destroyed_format) == 0x7b0a);
static_assert(offsetof(DataSegment, air_strike_name) == 0x7b28);
static_assert(offsetof(DataSegment, air_strike_begin) == 0x7b33);
static_assert(offsetof(DataSegment, air_strike_spotted) == 0x7b49);
static_assert(offsetof(DataSegment, air_bombers_name) == 0x7b67);
static_assert(offsetof(DataSegment, paratroop_drop_name) == 0x7b6f);
static_assert(offsetof(DataSegment, paratroop_drop_begin) == 0x7b7e);
static_assert(offsetof(DataSegment, paratroop_drop_spotted) == 0x7b90);
static_assert(offsetof(DataSegment, paratroop_planes_name) == 0x7ba9);
static_assert(offsetof(DataSegment, cargo_drop_name) == 0x7bb4);
static_assert(offsetof(DataSegment, cargo_drop_begin) == 0x7bbf);
static_assert(offsetof(DataSegment, cargo_drop_spotted) == 0x7bd2);
static_assert(offsetof(DataSegment, cargo_planes_name) == 0x7bec);
static_assert(offsetof(DataSegment, cargo_unit_report) == 0x7bf9);
static_assert(offsetof(DataSegment, cargo_city_report) == 0x7c2f);
static_assert(offsetof(DataSegment, unknown_7c65) == 0x7c65);
static_assert(offsetof(DataSegment, visit_format) == 0x7c66);
static_assert(offsetof(DataSegment, text_7c7f) == 0x7c7f);
static_assert(offsetof(DataSegment, text_7c89) == 0x7c89);
static_assert(offsetof(DataSegment, unknown_7c8f) == 0x7c8f);
static_assert(offsetof(DataSegment, hurricane_words) == 0x7c90);
static_assert(offsetof(DataSegment, text_7c9e) == 0x7c9e);
static_assert(offsetof(DataSegment, partisan_words) == 0x7ca4);
static_assert(offsetof(DataSegment, unknown_7ca8) == 0x7ca8);
static_assert(offsetof(DataSegment, text_7cb1) == 0x7cb1);
static_assert(offsetof(DataSegment, resistance_names) == 0x7cbc);
static_assert(offsetof(DataSegment, angel_name) == 0x7cc0);
static_assert(offsetof(DataSegment, angel_format) == 0x7cdd);
static_assert(offsetof(DataSegment, angel_actions) == 0x7cec);
static_assert(offsetof(DataSegment, spy_name_format) == 0x7cf8);
static_assert(offsetof(DataSegment, spy_laugh_text) == 0x7d07);
static_assert(offsetof(DataSegment, spy_move_text) == 0x7d1a);
static_assert(offsetof(DataSegment, spy_bold_off) == 0x7d3c);
static_assert(offsetof(DataSegment, spy_humph_text) == 0x7d40);
static_assert(offsetof(DataSegment, death_prefix) == 0x7d4a);
static_assert(offsetof(DataSegment, death_suffix) == 0x7d55);
static_assert(offsetof(DataSegment, comms_restored_format) == 0x7d77);
static_assert(offsetof(DataSegment, spy_captured_format) == 0x7da5);
static_assert(offsetof(DataSegment, comms_unit_text) == 0x7dd4);
static_assert(offsetof(DataSegment, spy_unit_text) == 0x7dfa);
static_assert(offsetof(DataSegment, unknown_7e17) == 0x7e17);
static_assert(offsetof(DataSegment, quantity_divisors) == 0x7e18);
static_assert(offsetof(DataSegment, request_supplies_name) == 0x7e28);
static_assert(offsetof(DataSegment, request_battalions_name) == 0x7e31);
static_assert(offsetof(DataSegment, request_regiments_name) == 0x7e3c);
static_assert(offsetof(DataSegment, request_kind_names) == 0x7e46);
static_assert(offsetof(DataSegment, request_delay_bases) == 0x7e4c);
static_assert(offsetof(DataSegment, equipment_delivery_format) == 0x7e4f);
static_assert(offsetof(DataSegment, request_weights2) == 0x7e5c);
static_assert(offsetof(DataSegment, context_name_format) == 0x7e62);
static_assert(offsetof(DataSegment, context_stats_format) == 0x7e79);
static_assert(offsetof(DataSegment, context_blank_format) == 0x7f0b);
static_assert(offsetof(DataSegment, context_text_padding) == 0x7f1b);
static_assert(offsetof(DataSegment, scenario_contexts) == 0x7f1c);
static_assert(offsetof(DataSegment, menu_pass_label) == 0x7f22);
static_assert(offsetof(DataSegment, menu_tact_label) == 0x7f29);
static_assert(offsetof(DataSegment, menu_city_label) == 0x7f30);
static_assert(offsetof(DataSegment, menu_tsfr_label) == 0x7f37);
static_assert(offsetof(DataSegment, menu_ofcr_label) == 0x7f3d);
static_assert(offsetof(DataSegment, menu_info_label) == 0x7f44);
static_assert(offsetof(DataSegment, menu_orgz_label) == 0x7f4b);
static_assert(offsetof(DataSegment, menu_rqst_label) == 0x7f52);
static_assert(offsetof(DataSegment, menu_air_label) == 0x7f59);
static_assert(offsetof(DataSegment, menu_spfc_label) == 0x7f5f);
static_assert(offsetof(DataSegment, menu_game_label) == 0x7f66);
static_assert(offsetof(DataSegment, control_prompt) == 0x7f6d);
static_assert(offsetof(DataSegment, control_ready) == 0x7f8c);
static_assert(offsetof(DataSegment, control_text_padding) == 0x7fb3);
static_assert(offsetof(DataSegment, control_actions) == 0x7fb4);
static_assert(offsetof(DataSegment, control_menu) == 0x7fe0);
static_assert(offsetof(DataSegment, turn_text) == 0x8070);
static_assert(offsetof(DataSegment, direction_keys) == 0x8070);
static_assert(offsetof(DataSegment, turn_label_format) == 0x8078);
static_assert(offsetof(DataSegment, percent_text) == 0x80a8);
static_assert(offsetof(DataSegment, not_available_text) == 0x80ae);
static_assert(offsetof(DataSegment, text_80b3) == 0x80b3);
static_assert(offsetof(DataSegment, unknown_80b5) == 0x80b5);
static_assert(offsetof(DataSegment, flash_state) == 0x80b6);
static_assert(offsetof(DataSegment, regiment_position_text) == 0x80b8);
static_assert(offsetof(DataSegment, division_position_text) == 0x80e8);
static_assert(offsetof(DataSegment, order_glyphs) == 0x8116);
static_assert(offsetof(DataSegment, view_clear_pattern) == 0x8124);
static_assert(offsetof(DataSegment, panel_rows) == 0x8136);
static_assert(offsetof(DataSegment, panel_columns) == 0x813c);
static_assert(offsetof(DataSegment, hurricane_name) == 0x8142);
static_assert(offsetof(DataSegment, turn_date_format) == 0x814c);
static_assert(offsetof(DataSegment, turn_panel_text) == 0x816b);
static_assert(offsetof(DataSegment, context_panel_text) == 0x816f);
static_assert(offsetof(DataSegment, info_panel_text) == 0x8173);
static_assert(offsetof(DataSegment, title_panel_text) == 0x8177);
static_assert(offsetof(DataSegment, message_panel_text) == 0x817b);
static_assert(offsetof(DataSegment, title_format) == 0x817f);
static_assert(offsetof(DataSegment, speaker_text) == 0x8188);
static_assert(offsetof(DataSegment, division_title) == 0x818f);
static_assert(offsetof(DataSegment, commander_title) == 0x819b);
static_assert(offsetof(DataSegment, speaker_name_format) == 0x81a5);
static_assert(offsetof(DataSegment, speaker_body_text) == 0x81aa);
static_assert(offsetof(DataSegment, speaker_end_text) == 0x81b4);
static_assert(offsetof(DataSegment, menu_prompt_format) == 0x81bb);
static_assert(offsetof(DataSegment, confirm_space) == 0x81be);
static_assert(offsetof(DataSegment, confirm_ok_text) == 0x81c0);
static_assert(offsetof(DataSegment, delay_hours_format) == 0x81cc);
static_assert(offsetof(DataSegment, unit_report_format) == 0x81f7);
static_assert(offsetof(DataSegment, objective_report_format) == 0x8212);
static_assert(offsetof(DataSegment, attack_format) == 0x8229);
static_assert(offsetof(DataSegment, defense_format) == 0x8248);
static_assert(offsetof(DataSegment, speed_format) == 0x8256);
static_assert(offsetof(DataSegment, move_range_format) == 0x8265);
static_assert(offsetof(DataSegment, gun_range_format) == 0x8274);
static_assert(offsetof(DataSegment, group_element_format) == 0x8289);
static_assert(offsetof(DataSegment, element_format) == 0x830d);
static_assert(offsetof(DataSegment, cursor_position_format) == 0x833e);
static_assert(offsetof(DataSegment, cursor_unit_format) == 0x834f);
static_assert(offsetof(DataSegment, cursor_end) == 0x8358);
static_assert(offsetof(DataSegment, pick_direction_keys) == 0x8360);
static_assert(offsetof(DataSegment, general_wounded) == 0x8368);
static_assert(offsetof(DataSegment, general_killed) == 0x8370);
static_assert(offsetof(DataSegment, unknown_8377) == 0x8377);
static_assert(offsetof(DataSegment, general_casualty_format) == 0x8378);
static_assert(offsetof(DataSegment, select_category_ranks) == 0x838e);
static_assert(offsetof(DataSegment, order_text) == 0x8396);
static_assert(offsetof(DataSegment, casualty_heading) == 0x8396);
static_assert(offsetof(DataSegment, casualty_cut_selected) == 0x83a7);
static_assert(offsetof(DataSegment, casualty_cut) == 0x83b9);
static_assert(offsetof(DataSegment, casualty_loss_format) == 0x83dc);
static_assert(offsetof(DataSegment, casualty_losses_format) == 0x83f0);
static_assert(offsetof(DataSegment, casualty_wiped) == 0x8406);
static_assert(offsetof(DataSegment, casualty_no_damage) == 0x8417);
static_assert(offsetof(DataSegment, damage_supply_format) == 0x842d);
static_assert(offsetof(DataSegment, damage_title) == 0x8459);
static_assert(offsetof(DataSegment, damage_heading) == 0x846f);
static_assert(offsetof(DataSegment, damage_element_format) == 0x8491);
static_assert(offsetof(DataSegment, damage_none) == 0x849f);
static_assert(offsetof(DataSegment, damage_loss_one) == 0x84b5);
static_assert(offsetof(DataSegment, damage_loss_many) == 0x84c9);
static_assert(offsetof(DataSegment, damage_endurance_format) == 0x84df);
static_assert(offsetof(DataSegment, unknown_8509) == 0x8509);
static_assert(offsetof(DataSegment, category_columns) == 0x850a);
static_assert(offsetof(DataSegment, ai_categories) == 0x8512);
static_assert(offsetof(DataSegment, target_text) == 0x8518);
static_assert(offsetof(DataSegment, target_two_line_format) == 0x852b);
static_assert(offsetof(DataSegment, target_one_line_format) == 0x8532);
static_assert(offsetof(DataSegment, target_cell_format) == 0x8539);
static_assert(offsetof(DataSegment, target_cancel_text) == 0x854a);
static_assert(offsetof(DataSegment, cancel_orders_prompt) == 0x8569);
static_assert(offsetof(DataSegment, text_8578) == 0x8578);
static_assert(offsetof(DataSegment, text_8580) == 0x8580);
static_assert(offsetof(DataSegment, text_8588) == 0x8588);
static_assert(offsetof(DataSegment, text_8590) == 0x8590);
static_assert(offsetof(DataSegment, text_8598) == 0x8598);
static_assert(offsetof(DataSegment, text_85a0) == 0x85a0);
static_assert(offsetof(DataSegment, text_85a8) == 0x85a8);
static_assert(offsetof(DataSegment, text_85b0) == 0x85b0);
static_assert(offsetof(DataSegment, text_85b8) == 0x85b8);
static_assert(offsetof(DataSegment, text_85c0) == 0x85c0);
static_assert(offsetof(DataSegment, text_85c8) == 0x85c8);
static_assert(offsetof(DataSegment, text_85d0) == 0x85d0);
static_assert(offsetof(DataSegment, text_85d8) == 0x85d8);
static_assert(offsetof(DataSegment, text_85e0) == 0x85e0);
static_assert(offsetof(DataSegment, text_85e8) == 0x85e8);
static_assert(offsetof(DataSegment, text_85f0) == 0x85f0);
static_assert(offsetof(DataSegment, text_85f8) == 0x85f8);
static_assert(offsetof(DataSegment, text_8600) == 0x8600);
static_assert(offsetof(DataSegment, text_8608) == 0x8608);
static_assert(offsetof(DataSegment, text_8610) == 0x8610);
static_assert(offsetof(DataSegment, text_8618) == 0x8618);
static_assert(offsetof(DataSegment, text_8620) == 0x8620);
static_assert(offsetof(DataSegment, text_8628) == 0x8628);
static_assert(offsetof(DataSegment, text_8630) == 0x8630);
static_assert(offsetof(DataSegment, text_8638) == 0x8638);
static_assert(offsetof(DataSegment, text_8641) == 0x8641);
static_assert(offsetof(DataSegment, text_864a) == 0x864a);
static_assert(offsetof(DataSegment, text_8657) == 0x8657);
static_assert(offsetof(DataSegment, order_menus) == 0x8664);
static_assert(offsetof(DataSegment, supply_choices) == 0x8814);
static_assert(offsetof(DataSegment, repair_choices) == 0x8838);
static_assert(offsetof(DataSegment, order_spots) == 0x885c);
static_assert(offsetof(DataSegment, choice_spots) == 0x887c);
static_assert(offsetof(DataSegment, issue_prompts) == 0x8884);
static_assert(offsetof(DataSegment, issue_order_codes) == 0x89bc);
static_assert(offsetof(DataSegment, awaiting_orders) == 0x89dc);
static_assert(offsetof(DataSegment, action_prompts) == 0x89f7);
static_assert(offsetof(DataSegment, unknown_8b2f) == 0x8b2f);
static_assert(offsetof(DataSegment, action_order_codes) == 0x8b30);
static_assert(offsetof(DataSegment, select_division) == 0x8b48);
static_assert(offsetof(DataSegment, select_order) == 0x8b60);
static_assert(offsetof(DataSegment, order_whole_division) == 0x8b75);
static_assert(offsetof(DataSegment, select_regiment) == 0x8ba0);
static_assert(offsetof(DataSegment, stock_header) == 0x8bb8);
static_assert(offsetof(DataSegment, stock_fuel_line) == 0x8bdf);
static_assert(offsetof(DataSegment, stock_ammo_line) == 0x8bfe);
static_assert(offsetof(DataSegment, stock_food_line) == 0x8c1d);
static_assert(offsetof(DataSegment, stock_question_short) == 0x8c42);
static_assert(offsetof(DataSegment, stock_question) == 0x8c63);
static_assert(offsetof(DataSegment, stock_fuel) == 0x8c9a);
static_assert(offsetof(DataSegment, stock_ammo) == 0x8c9f);
static_assert(offsetof(DataSegment, stock_food) == 0x8ca4);
static_assert(offsetof(DataSegment, stock_no_cities) == 0x8ca9);
static_assert(offsetof(DataSegment, stock_select_city) == 0x8cbf);
static_assert(offsetof(DataSegment, transfer_which_division) == 0x8cce);
static_assert(offsetof(DataSegment, transfer_which_regiment) == 0x8cf2);
static_assert(offsetof(DataSegment, transfer_which_group) == 0x8d02);
static_assert(offsetof(DataSegment, transfer_hour) == 0x8d0f);
static_assert(offsetof(DataSegment, transfer_hours_text) == 0x8d14);
static_assert(offsetof(DataSegment, transfer_confirm) == 0x8d1a);
static_assert(offsetof(DataSegment, rest_thanks_text) == 0x8d4c);
static_assert(offsetof(DataSegment, assign_thanks_text) == 0x8d69);
static_assert(offsetof(DataSegment, division_charge_prompt) == 0x8d83);
static_assert(offsetof(DataSegment, post_label_none) == 0x8dba);
static_assert(offsetof(DataSegment, text_8dc0) == 0x8dc0);
static_assert(offsetof(DataSegment, text_8dc8) == 0x8dc8);
static_assert(offsetof(DataSegment, text_8dd0) == 0x8dd0);
static_assert(offsetof(DataSegment, text_8dd8) == 0x8dd8);
static_assert(offsetof(DataSegment, text_8de0) == 0x8de0);
static_assert(offsetof(DataSegment, text_8dea) == 0x8dea);
static_assert(offsetof(DataSegment, none_available_text) == 0x8df2);
static_assert(offsetof(DataSegment, assign_who_prompt) == 0x8e0e);
static_assert(offsetof(DataSegment, no_holder_1) == 0x8e23);
static_assert(offsetof(DataSegment, post_format_1) == 0x8e24);
static_assert(offsetof(DataSegment, no_holder_2) == 0x8e2b);
static_assert(offsetof(DataSegment, post_format_2) == 0x8e2c);
static_assert(offsetof(DataSegment, no_holder_3) == 0x8e33);
static_assert(offsetof(DataSegment, post_format_3) == 0x8e34);
static_assert(offsetof(DataSegment, no_holder_4) == 0x8e3b);
static_assert(offsetof(DataSegment, post_format_4) == 0x8e3c);
static_assert(offsetof(DataSegment, post_format_5) == 0x8e43);
static_assert(offsetof(DataSegment, post_format_6) == 0x8e4a);
static_assert(offsetof(DataSegment, assign_where_prompt) == 0x8e51);
static_assert(offsetof(DataSegment, assign_menu) == 0x8e72);
static_assert(offsetof(DataSegment, post_labels) == 0x8ec6);
static_assert(offsetof(DataSegment, inactive_division_label) == 0x8ed2);
static_assert(offsetof(DataSegment, none_to_rest_text) == 0x8edc);
static_assert(offsetof(DataSegment, rest_who_prompt) == 0x8f05);
static_assert(offsetof(DataSegment, rest_days_prompt) == 0x8f18);
static_assert(offsetof(DataSegment, text_8f45) == 0x8f45);
static_assert(offsetof(DataSegment, text_8f4e) == 0x8f4e);
static_assert(offsetof(DataSegment, text_8f56) == 0x8f56);
static_assert(offsetof(DataSegment, personnel_prompt) == 0x8f5b);
static_assert(offsetof(DataSegment, unknown_8f6b) == 0x8f6b);
static_assert(offsetof(DataSegment, personnel_actions) == 0x8f6c);
static_assert(offsetof(DataSegment, personnel_menu) == 0x8f78);
static_assert(offsetof(DataSegment, advice_tips) == 0x8fa8);
static_assert(offsetof(DataSegment, text_8ff8) == 0x8ff8);
static_assert(offsetof(DataSegment, text_9008) == 0x9008);
static_assert(offsetof(DataSegment, text_9018) == 0x9018);
static_assert(offsetof(DataSegment, text_9029) == 0x9029);
static_assert(offsetof(DataSegment, text_9035) == 0x9035);
static_assert(offsetof(DataSegment, advice_prompt) == 0x9045);
static_assert(offsetof(DataSegment, advice_actions) == 0x9060);
static_assert(offsetof(DataSegment, advice_menu) == 0x9074);
static_assert(offsetof(DataSegment, near_907c) == 0x907c);
static_assert(offsetof(DataSegment, bytes_907e) == 0x907e);
static_assert(offsetof(DataSegment, near_9088) == 0x9088);
static_assert(offsetof(DataSegment, bytes_908a) == 0x908a);
static_assert(offsetof(DataSegment, near_9094) == 0x9094);
static_assert(offsetof(DataSegment, bytes_9096) == 0x9096);
static_assert(offsetof(DataSegment, near_90a0) == 0x90a0);
static_assert(offsetof(DataSegment, bytes_90a2) == 0x90a2);
static_assert(offsetof(DataSegment, near_90ac) == 0x90ac);
static_assert(offsetof(DataSegment, bytes_90ae) == 0x90ae);
static_assert(offsetof(DataSegment, text_90bc) == 0x90bc);
static_assert(offsetof(DataSegment, text_90c7) == 0x90c7);
static_assert(offsetof(DataSegment, staff_prompt) == 0x90d4);
static_assert(offsetof(DataSegment, staff_actions) == 0x90e2);
static_assert(offsetof(DataSegment, staff_menu) == 0x90ee);
static_assert(offsetof(DataSegment, near_90f6) == 0x90f6);
static_assert(offsetof(DataSegment, bytes_90f8) == 0x90f8);
static_assert(offsetof(DataSegment, near_9102) == 0x9102);
static_assert(offsetof(DataSegment, bytes_9104) == 0x9104);
static_assert(offsetof(DataSegment, supply_text) == 0x9112);
static_assert(offsetof(DataSegment, panel_fuel_ammo) == 0x9112);
static_assert(offsetof(DataSegment, panel_food_end) == 0x9147);
static_assert(offsetof(DataSegment, panel_garrison) == 0x9179);
static_assert(offsetof(DataSegment, panel_stock_row) == 0x919a);
static_assert(offsetof(DataSegment, panel_no_stock) == 0x91b6);
static_assert(offsetof(DataSegment, unknown_91c3) == 0x91c3);
static_assert(offsetof(DataSegment, stock_rows) == 0x91c4);
static_assert(offsetof(DataSegment, panel_waiting) == 0x91dc);
static_assert(offsetof(DataSegment, panel_orders) == 0x91e4);
static_assert(offsetof(DataSegment, panel_target) == 0x9219);
static_assert(offsetof(DataSegment, panel_fatigue) == 0x924c);
static_assert(offsetof(DataSegment, panel_element_row) == 0x9281);
static_assert(offsetof(DataSegment, unknown_929d) == 0x929d);
static_assert(offsetof(DataSegment, element_rows) == 0x929e);
static_assert(offsetof(DataSegment, text_92b6) == 0x92b6);
static_assert(offsetof(DataSegment, text_92c3) == 0x92c3);
static_assert(offsetof(DataSegment, text_92cf) == 0x92cf);
static_assert(offsetof(DataSegment, text_92db) == 0x92db);
static_assert(offsetof(DataSegment, contact_prompt) == 0x92e7);
static_assert(offsetof(DataSegment, contact_lost) == 0x9317);
static_assert(offsetof(DataSegment, unknown_9343) == 0x9343);
static_assert(offsetof(DataSegment, contact_menu) == 0x9344);
static_assert(offsetof(DataSegment, near_934c) == 0x934c);
static_assert(offsetof(DataSegment, bytes_934e) == 0x934e);
static_assert(offsetof(DataSegment, near_9358) == 0x9358);
static_assert(offsetof(DataSegment, bytes_935a) == 0x935a);
static_assert(offsetof(DataSegment, near_9364) == 0x9364);
static_assert(offsetof(DataSegment, bytes_9366) == 0x9366);
static_assert(offsetof(DataSegment, near_9370) == 0x9370);
static_assert(offsetof(DataSegment, bytes_9372) == 0x9372);
static_assert(offsetof(DataSegment, people_here) == 0x9380);
static_assert(offsetof(DataSegment, people_none) == 0x9391);
static_assert(offsetof(DataSegment, text_93a8) == 0x93a8);
static_assert(offsetof(DataSegment, text_93b1) == 0x93b1);
static_assert(offsetof(DataSegment, people_prompt) == 0x93b7);
static_assert(offsetof(DataSegment, people_actions) == 0x93d6);
static_assert(offsetof(DataSegment, people_menu) == 0x93de);
static_assert(offsetof(DataSegment, near_93e6) == 0x93e6);
static_assert(offsetof(DataSegment, bytes_93e8) == 0x93e8);
static_assert(offsetof(DataSegment, near_93f2) == 0x93f2);
static_assert(offsetof(DataSegment, bytes_93f4) == 0x93f4);
static_assert(offsetof(DataSegment, city_prompt) == 0x9402);
static_assert(offsetof(DataSegment, city_format) == 0x941c);
static_assert(offsetof(DataSegment, city_none_own) == 0x941f);
static_assert(offsetof(DataSegment, city_none_enemy) == 0x943e);
static_assert(offsetof(DataSegment, text_945c) == 0x945c);
static_assert(offsetof(DataSegment, text_9465) == 0x9465);
static_assert(offsetof(DataSegment, city_side_prompt) == 0x946b);
static_assert(offsetof(DataSegment, city_side_prompt_bytes) == 0x946b);
static_assert(offsetof(DataSegment, unknown_9489) == 0x9489);
static_assert(offsetof(DataSegment, city_actions) == 0x948a);
static_assert(offsetof(DataSegment, city_side_menu) == 0x9492);
static_assert(offsetof(DataSegment, near_949a) == 0x949a);
static_assert(offsetof(DataSegment, bytes_949c) == 0x949c);
static_assert(offsetof(DataSegment, near_94a6) == 0x94a6);
static_assert(offsetof(DataSegment, bytes_94a8) == 0x94a8);
static_assert(offsetof(DataSegment, regiment_prompt) == 0x94b6);
static_assert(offsetof(DataSegment, regiment_format) == 0x94d9);
static_assert(offsetof(DataSegment, view_division_prompt) == 0x94dc);
static_assert(offsetof(DataSegment, units_none_enemy) == 0x94fa);
static_assert(offsetof(DataSegment, text_9515) == 0x9515);
static_assert(offsetof(DataSegment, text_951e) == 0x951e);
static_assert(offsetof(DataSegment, info_prompt) == 0x9524);
static_assert(offsetof(DataSegment, info_actions) == 0x9542);
static_assert(offsetof(DataSegment, info_menu) == 0x954a);
static_assert(offsetof(DataSegment, near_9552) == 0x9552);
static_assert(offsetof(DataSegment, bytes_9554) == 0x9554);
static_assert(offsetof(DataSegment, near_955e) == 0x955e);
static_assert(offsetof(DataSegment, bytes_9560) == 0x9560);
static_assert(offsetof(DataSegment, weapons_prompt) == 0x956e);
static_assert(offsetof(DataSegment, weapons_here) == 0x958b);
static_assert(offsetof(DataSegment, text_959e) == 0x959e);
static_assert(offsetof(DataSegment, text_95a7) == 0x95a7);
static_assert(offsetof(DataSegment, arms_prompt) == 0x95ad);
static_assert(offsetof(DataSegment, arms_prompt_bytes) == 0x95ad);
static_assert(offsetof(DataSegment, unknown_95c1) == 0x95c1);
static_assert(offsetof(DataSegment, arms_actions) == 0x95c2);
static_assert(offsetof(DataSegment, arms_menu) == 0x95ca);
static_assert(offsetof(DataSegment, near_95d2) == 0x95d2);
static_assert(offsetof(DataSegment, bytes_95d4) == 0x95d4);
static_assert(offsetof(DataSegment, near_95de) == 0x95de);
static_assert(offsetof(DataSegment, bytes_95e0) == 0x95e0);
static_assert(offsetof(DataSegment, victory_title) == 0x95ee);
static_assert(offsetof(DataSegment, victory_format) == 0x9601);
static_assert(offsetof(DataSegment, victory_orders) == 0x9610);
static_assert(offsetof(DataSegment, text_962c) == 0x962c);
static_assert(offsetof(DataSegment, text_9636) == 0x9636);
static_assert(offsetof(DataSegment, text_963f) == 0x963f);
static_assert(offsetof(DataSegment, text_9646) == 0x9646);
static_assert(offsetof(DataSegment, text_964c) == 0x964c);
static_assert(offsetof(DataSegment, text_9656) == 0x9656);
static_assert(offsetof(DataSegment, view_main_prompt) == 0x9669);
static_assert(offsetof(DataSegment, view_main_prompt_bytes) == 0x9669);
static_assert(offsetof(DataSegment, unknown_968d) == 0x968d);
static_assert(offsetof(DataSegment, view_main_actions) == 0x968e);
static_assert(offsetof(DataSegment, view_main_items) == 0x96a6);
static_assert(offsetof(DataSegment, near_96ae) == 0x96ae);
static_assert(offsetof(DataSegment, bytes_96b0) == 0x96b0);
static_assert(offsetof(DataSegment, near_96ba) == 0x96ba);
static_assert(offsetof(DataSegment, bytes_96bc) == 0x96bc);
static_assert(offsetof(DataSegment, near_96c6) == 0x96c6);
static_assert(offsetof(DataSegment, bytes_96c8) == 0x96c8);
static_assert(offsetof(DataSegment, near_96d2) == 0x96d2);
static_assert(offsetof(DataSegment, bytes_96d4) == 0x96d4);
static_assert(offsetof(DataSegment, near_96de) == 0x96de);
static_assert(offsetof(DataSegment, bytes_96e0) == 0x96e0);
static_assert(offsetof(DataSegment, near_96ea) == 0x96ea);
static_assert(offsetof(DataSegment, bytes_96ec) == 0x96ec);
static_assert(offsetof(DataSegment, ops_chief_format) == 0x96fa);
static_assert(offsetof(DataSegment, number_range_format) == 0x9717);
static_assert(offsetof(DataSegment, supply_unlimited) == 0x9720);
static_assert(offsetof(DataSegment, supply_panel_x) == 0x9722);
static_assert(offsetof(DataSegment, supply_panel_y) == 0x9724);
static_assert(offsetof(DataSegment, supply_name_format) == 0x9726);
static_assert(offsetof(DataSegment, supply_name_text) == 0x9735);
static_assert(offsetof(DataSegment, text_9741) == 0x9741);
static_assert(offsetof(DataSegment, text_9744) == 0x9744);
static_assert(offsetof(DataSegment, text_9747) == 0x9747);
static_assert(offsetof(DataSegment, supply_row_left) == 0x974a);
static_assert(offsetof(DataSegment, supply_row_right) == 0x9754);
static_assert(offsetof(DataSegment, supply_row_end) == 0x9758);
static_assert(offsetof(DataSegment, unknown_975f) == 0x975f);
static_assert(offsetof(DataSegment, near_9760) == 0x9760);
static_assert(offsetof(DataSegment, near_9762) == 0x9762);
static_assert(offsetof(DataSegment, near_9764) == 0x9764);
static_assert(offsetof(DataSegment, supply_prompt) == 0x9766);
static_assert(offsetof(DataSegment, supply_continue) == 0x977e);
static_assert(offsetof(DataSegment, tank_suffix) == 0x9788);
static_assert(offsetof(DataSegment, nation_codes) == 0x978e);
static_assert(offsetof(DataSegment, nation_code_other) == 0x97aa);
static_assert(offsetof(DataSegment, text_97af) == 0x97af);
static_assert(offsetof(DataSegment, unknown_97b3) == 0x97b3);
static_assert(offsetof(DataSegment, supply_type_list) == 0x97b4);
static_assert(offsetof(DataSegment, supply_header_a) == 0x97c7);
static_assert(offsetof(DataSegment, supply_row_a) == 0x97e5);
static_assert(offsetof(DataSegment, text_9805) == 0x9805);
static_assert(offsetof(DataSegment, supply_empty_a) == 0x9809);
static_assert(offsetof(DataSegment, unknown_9831) == 0x9831);
static_assert(offsetof(DataSegment, supply_slot_list) == 0x9832);
static_assert(offsetof(DataSegment, supply_header_b) == 0x9845);
static_assert(offsetof(DataSegment, supply_row_b) == 0x9865);
static_assert(offsetof(DataSegment, supply_empty_b) == 0x987c);
static_assert(offsetof(DataSegment, alloc_amount_format) == 0x989a);
static_assert(offsetof(DataSegment, alloc_first_format) == 0x98ab);
static_assert(offsetof(DataSegment, alloc_second_format) == 0x98ba);
static_assert(offsetof(DataSegment, alloc_heading) == 0x98c9);
static_assert(offsetof(DataSegment, alloc_row_format) == 0x98e6);
static_assert(offsetof(DataSegment, alloc_name_format) == 0x98fc);
static_assert(offsetof(DataSegment, alloc_stats_format) == 0x990b);
static_assert(offsetof(DataSegment, regiment_hq_prompt) == 0x9987);
static_assert(offsetof(DataSegment, organize_none) == 0x99a6);
static_assert(offsetof(DataSegment, organize_which) == 0x99c6);
static_assert(offsetof(DataSegment, organize_confirm_format) == 0x99df);
static_assert(offsetof(DataSegment, organize_no_arms) == 0x99f9);
static_assert(offsetof(DataSegment, organize_no_neighbor) == 0x9a0d);
static_assert(offsetof(DataSegment, organize_with) == 0x9a3e);
static_assert(offsetof(DataSegment, organize_full) == 0x9a5c);
static_assert(offsetof(DataSegment, organize_dispatch) == 0x9a84);
static_assert(offsetof(DataSegment, organize_complete) == 0x9a94);
static_assert(offsetof(DataSegment, organize_cancel) == 0x9ada);
static_assert(offsetof(DataSegment, organize_select) == 0x9aee);
static_assert(offsetof(DataSegment, organize_number) == 0x9b02);
static_assert(offsetof(DataSegment, rq_no_more_text) == 0x9b1a);
static_assert(offsetof(DataSegment, rq_cancel_text) == 0x9b42);
static_assert(offsetof(DataSegment, rq_filed_text) == 0x9b75);
static_assert(offsetof(DataSegment, rq_supplies_text) == 0x9b9a);
static_assert(offsetof(DataSegment, rq_supplies_title) == 0x9ba3);
static_assert(offsetof(DataSegment, rq_supplies_where) == 0x9bb4);
static_assert(offsetof(DataSegment, rq_supplies_confirm) == 0x9be7);
static_assert(offsetof(DataSegment, text_9c01) == 0x9c01);
static_assert(offsetof(DataSegment, unknown_9c09) == 0x9c09);
static_assert(offsetof(DataSegment, rq_type_dialog) == 0x9c0a);
static_assert(offsetof(DataSegment, rq_type_header) == 0x9c1d);
static_assert(offsetof(DataSegment, rq_type_name) == 0x9c43);
static_assert(offsetof(DataSegment, rq_type_row) == 0x9c4c);
static_assert(offsetof(DataSegment, rq_arms_text) == 0x9c6f);
static_assert(offsetof(DataSegment, rq_arms_which) == 0x9c74);
static_assert(offsetof(DataSegment, rq_arms_where) == 0x9ca0);
static_assert(offsetof(DataSegment, rq_arms_confirm) == 0x9cd2);
static_assert(offsetof(DataSegment, text_9cec) == 0x9cec);
static_assert(offsetof(DataSegment, rq_regiment_list) == 0x9cf2);
static_assert(offsetof(DataSegment, rq_unit_header) == 0x9d05);
static_assert(offsetof(DataSegment, rq_unit_name) == 0x9d1d);
static_assert(offsetof(DataSegment, rq_unit_row) == 0x9d26);
static_assert(offsetof(DataSegment, text_9d3f) == 0x9d3f);
static_assert(offsetof(DataSegment, text_9d48) == 0x9d48);
static_assert(offsetof(DataSegment, text_9d53) == 0x9d53);
static_assert(offsetof(DataSegment, text_9d5b) == 0x9d5b);
static_assert(offsetof(DataSegment, rq_reinforcements_text) == 0x9d65);
static_assert(offsetof(DataSegment, rq_regiment_text) == 0x9d74);
static_assert(offsetof(DataSegment, rq_battalion_text) == 0x9d7d);
static_assert(offsetof(DataSegment, rq_which_regiment_text) == 0x9d87);
static_assert(offsetof(DataSegment, rq_regiments_text) == 0x9dae);
static_assert(offsetof(DataSegment, rq_regiment_kind_text) == 0x9db8);
static_assert(offsetof(DataSegment, unknown_9dd3) == 0x9dd3);
static_assert(offsetof(DataSegment, rq_slash_text) == 0x9dd4);
static_assert(offsetof(DataSegment, near_9ddc) == 0x9ddc);
static_assert(offsetof(DataSegment, bytes_9dde) == 0x9dde);
static_assert(offsetof(DataSegment, near_9de8) == 0x9de8);
static_assert(offsetof(DataSegment, bytes_9dea) == 0x9dea);
static_assert(offsetof(DataSegment, near_9df4) == 0x9df4);
static_assert(offsetof(DataSegment, bytes_9df6) == 0x9df6);
static_assert(offsetof(DataSegment, near_9e00) == 0x9e00);
static_assert(offsetof(DataSegment, bytes_9e02) == 0x9e02);
static_assert(offsetof(DataSegment, air_text) == 0x9e10);
static_assert(offsetof(DataSegment, rq_header_text) == 0x9e10);
static_assert(offsetof(DataSegment, text_9e3b) == 0x9e3b);
static_assert(offsetof(DataSegment, text_9e44) == 0x9e44);
static_assert(offsetof(DataSegment, text_9e4f) == 0x9e4f);
static_assert(offsetof(DataSegment, text_9e55) == 0x9e55);
static_assert(offsetof(DataSegment, text_9e5f) == 0x9e5f);
static_assert(offsetof(DataSegment, text_9e68) == 0x9e68);
static_assert(offsetof(DataSegment, text_9e6f) == 0x9e6f);
static_assert(offsetof(DataSegment, text_9e78) == 0x9e78);
static_assert(offsetof(DataSegment, text_9e83) == 0x9e83);
static_assert(offsetof(DataSegment, rq_title_text) == 0x9e8d);
static_assert(offsetof(DataSegment, rq_supply_row_text) == 0x9e95);
static_assert(offsetof(DataSegment, rq_supply_filed_text) == 0x9ec1);
static_assert(offsetof(DataSegment, rq_battalion_row_text) == 0x9eed);
static_assert(offsetof(DataSegment, rq_battalion_filed_text) == 0x9f18);
static_assert(offsetof(DataSegment, rq_regiment_row_text) == 0x9f42);
static_assert(offsetof(DataSegment, rq_regiment_filed_text) == 0x9f71);
static_assert(offsetof(DataSegment, rq_menu_prompt_text) == 0x9f9f);
static_assert(offsetof(DataSegment, rq_busy_text) == 0x9fb1);
static_assert(offsetof(DataSegment, rq_no_city_text) == 0x9ff0);
static_assert(offsetof(DataSegment, unknown_a00f) == 0xa00f);
static_assert(offsetof(DataSegment, rq_regiment_kind_names) == 0xa010);
static_assert(offsetof(DataSegment, bytes_a01c) == 0xa01c);
static_assert(offsetof(DataSegment, request_handlers) == 0xa01e);
static_assert(offsetof(DataSegment, rq_menu_items) == 0xa02a);
static_assert(offsetof(DataSegment, near_a032) == 0xa032);
static_assert(offsetof(DataSegment, bytes_a034) == 0xa034);
static_assert(offsetof(DataSegment, near_a03e) == 0xa03e);
static_assert(offsetof(DataSegment, bytes_a040) == 0xa040);
static_assert(offsetof(DataSegment, near_a04a) == 0xa04a);
static_assert(offsetof(DataSegment, bytes_a04c) == 0xa04c);
static_assert(offsetof(DataSegment, air_question_mark) == 0xa05a);
static_assert(offsetof(DataSegment, air_range_format) == 0xa05d);
static_assert(offsetof(DataSegment, air_input_begin) == 0xa06a);
static_assert(offsetof(DataSegment, air_input_end) == 0xa06e);
static_assert(offsetof(DataSegment, air_hostile_city) == 0xa072);
static_assert(offsetof(DataSegment, air_understood) == 0xa07f);
static_assert(offsetof(DataSegment, text_a098) == 0xa098);
static_assert(offsetof(DataSegment, text_a0a9) == 0xa0a9);
static_assert(offsetof(DataSegment, text_a0ba) == 0xa0ba);
static_assert(offsetof(DataSegment, air_plane) == 0xa0c5);
static_assert(offsetof(DataSegment, air_planes) == 0xa0cb);
static_assert(offsetof(DataSegment, air_hour) == 0xa0d2);
static_assert(offsetof(DataSegment, air_hours) == 0xa0d7);
static_assert(offsetof(DataSegment, unknown_a0dd) == 0xa0dd);
static_assert(offsetof(DataSegment, bombing_flights) == 0xa0de);
static_assert(offsetof(DataSegment, bombing_names) == 0xa0e4);
static_assert(offsetof(DataSegment, bytes_a0ea) == 0xa0ea);
static_assert(offsetof(DataSegment, text_a108) == 0xa108);
static_assert(offsetof(DataSegment, text_a125) == 0xa125);
static_assert(offsetof(DataSegment, air_null_order) == 0xa142);
static_assert(offsetof(DataSegment, bombing_title) == 0xa14c);
static_assert(offsetof(DataSegment, bombing_site_prompt) == 0xa15b);
static_assert(offsetof(DataSegment, unknown_a171) == 0xa171);
static_assert(offsetof(DataSegment, bombing_menu) == 0xa172);
static_assert(offsetof(DataSegment, near_a186) == 0xa186);
static_assert(offsetof(DataSegment, words_a188) == 0xa188);
static_assert(offsetof(DataSegment, near_a192) == 0xa192);
static_assert(offsetof(DataSegment, bytes_a194) == 0xa194);
static_assert(offsetof(DataSegment, drop_ask_troops) == 0xa1a2);
static_assert(offsetof(DataSegment, drop_troops_assigned) == 0xa1c7);
static_assert(offsetof(DataSegment, drop_which_regiment) == 0xa1e1);
static_assert(offsetof(DataSegment, drop_assign_soldiers) == 0xa208);
static_assert(offsetof(DataSegment, drop_command_post) == 0xa222);
static_assert(offsetof(DataSegment, drop_understood) == 0xa24f);
static_assert(offsetof(DataSegment, drop_plane) == 0xa268);
static_assert(offsetof(DataSegment, drop_planes) == 0xa26e);
static_assert(offsetof(DataSegment, drop_no_paratroopers) == 0xa275);
static_assert(offsetof(DataSegment, drop_cancel) == 0xa297);
static_assert(offsetof(DataSegment, drop_no_transports) == 0xa2a7);
static_assert(offsetof(DataSegment, drop_no_regiments) == 0xa2d5);
static_assert(offsetof(DataSegment, drop_no_battalions) == 0xa2fe);
static_assert(offsetof(DataSegment, drop_where) == 0xa328);
static_assert(offsetof(DataSegment, drop_paratroopers) == 0xa355);
static_assert(offsetof(DataSegment, drop_hours_prompt) == 0xa362);
static_assert(offsetof(DataSegment, drop_ask_hq) == 0xa383);
static_assert(offsetof(DataSegment, lift_lead_format) == 0xa3ca);
static_assert(offsetof(DataSegment, lift_understood) == 0xa3f6);
static_assert(offsetof(DataSegment, air_lift_plane) == 0xa40f);
static_assert(offsetof(DataSegment, air_lift_planes) == 0xa415);
static_assert(offsetof(DataSegment, air_lift_hour) == 0xa41c);
static_assert(offsetof(DataSegment, air_lift_hours) == 0xa421);
static_assert(offsetof(DataSegment, lift_start_format) == 0xa427);
static_assert(offsetof(DataSegment, lift_cancel) == 0xa456);
static_assert(offsetof(DataSegment, lift_no_planes) == 0xa46a);
static_assert(offsetof(DataSegment, lift_ask_ammo) == 0xa499);
static_assert(offsetof(DataSegment, lift_ask_fuel) == 0xa4af);
static_assert(offsetof(DataSegment, lift_ask_food) == 0xa4c5);
static_assert(offsetof(DataSegment, lift_select_point) == 0xa4db);
static_assert(offsetof(DataSegment, lift_cargo_name) == 0xa4f4);
static_assert(offsetof(DataSegment, lift_ask_hours) == 0xa4fa);
static_assert(offsetof(DataSegment, lift_confirm) == 0xa511);
static_assert(offsetof(DataSegment, air_header_format) == 0xa552);
static_assert(offsetof(DataSegment, text_a56f) == 0xa56f);
static_assert(offsetof(DataSegment, text_a57a) == 0xa57a);
static_assert(offsetof(DataSegment, text_a587) == 0xa587);
static_assert(offsetof(DataSegment, air_title) == 0xa592);
static_assert(offsetof(DataSegment, air_strike_format) == 0xa59c);
static_assert(offsetof(DataSegment, air_drop_format) == 0xa5c6);
static_assert(offsetof(DataSegment, air_lift_format) == 0xa5f0);
static_assert(offsetof(DataSegment, air_mission_question) == 0xa61a);
static_assert(offsetof(DataSegment, unknown_a631) == 0xa631);
static_assert(offsetof(DataSegment, air_missions) == 0xa632);
static_assert(offsetof(DataSegment, air_mission_menu) == 0xa63e);
static_assert(offsetof(DataSegment, near_a646) == 0xa646);
static_assert(offsetof(DataSegment, bytes_a648) == 0xa648);
static_assert(offsetof(DataSegment, near_a652) == 0xa652);
static_assert(offsetof(DataSegment, bytes_a654) == 0xa654);
static_assert(offsetof(DataSegment, near_a65e) == 0xa65e);
static_assert(offsetof(DataSegment, bytes_a660) == 0xa660);
static_assert(offsetof(DataSegment, raid_title) == 0xa66e);
static_assert(offsetof(DataSegment, raid_prompt) == 0xa673);
static_assert(offsetof(DataSegment, raid_city_question) == 0xa686);
static_assert(offsetof(DataSegment, raid_unit_question) == 0xa697);
static_assert(offsetof(DataSegment, raid_refusal_1) == 0xa6a0);
static_assert(offsetof(DataSegment, raid_refusal_2) == 0xa6c7);
static_assert(offsetof(DataSegment, raid_refusal_3) == 0xa6fc);
static_assert(offsetof(DataSegment, raid_refusal_4) == 0xa72f);
static_assert(offsetof(DataSegment, raid_lieutenant) == 0xa753);
static_assert(offsetof(DataSegment, sabotage_title) == 0xa776);
static_assert(offsetof(DataSegment, sabotage_prompt) == 0xa77f);
static_assert(offsetof(DataSegment, sabotage_city_question) == 0xa7a5);
static_assert(offsetof(DataSegment, sabotage_refusal_1) == 0xa7b4);
static_assert(offsetof(DataSegment, sabotage_refusal_2) == 0xa7de);
static_assert(offsetof(DataSegment, sabotage_refusal_3) == 0xa80d);
static_assert(offsetof(DataSegment, sabotage_refusal_4) == 0xa83e);
static_assert(offsetof(DataSegment, sabotage_duration) == 0xa860);
static_assert(offsetof(DataSegment, sabotage_lieutenant) == 0xa89c);
static_assert(offsetof(DataSegment, cut_comm_title) == 0xa8c4);
static_assert(offsetof(DataSegment, cut_comm_prompt) == 0xa8cd);
static_assert(offsetof(DataSegment, cut_comm_city_question) == 0xa8ef);
static_assert(offsetof(DataSegment, cut_comm_unit_question) == 0xa90b);
static_assert(offsetof(DataSegment, cut_comm_refusal_1) == 0xa914);
static_assert(offsetof(DataSegment, cut_comm_refusal_2) == 0xa937);
static_assert(offsetof(DataSegment, cut_comm_refusal_3) == 0xa963);
static_assert(offsetof(DataSegment, cut_comm_refusal_4) == 0xa998);
static_assert(offsetof(DataSegment, cut_comm_hour) == 0xa9cd);
static_assert(offsetof(DataSegment, cut_comm_hours) == 0xa9d2);
static_assert(offsetof(DataSegment, cut_comm_duration) == 0xa9d8);
static_assert(offsetof(DataSegment, cut_comm_lieutenant) == 0xaa08);
static_assert(offsetof(DataSegment, assassinate_title) == 0xaa35);
static_assert(offsetof(DataSegment, assassinate_prompt) == 0xaa40);
static_assert(offsetof(DataSegment, assassinate_unit_question) == 0xaa5c);
static_assert(offsetof(DataSegment, assassinate_refusal_1) == 0xaa77);
static_assert(offsetof(DataSegment, assassinate_refusal_2) == 0xaa8d);
static_assert(offsetof(DataSegment, assassinate_refusal_3) == 0xaabc);
static_assert(offsetof(DataSegment, assassinate_refusal_4) == 0xaacf);
static_assert(offsetof(DataSegment, assassinate_duration) == 0xaafe);
static_assert(offsetof(DataSegment, assassinate_lieutenant) == 0xab38);
static_assert(offsetof(DataSegment, recon_title) == 0xab5b);
static_assert(offsetof(DataSegment, recon_prompt) == 0xab61);
static_assert(offsetof(DataSegment, recon_city_question) == 0xab78);
static_assert(offsetof(DataSegment, recon_unit_question) == 0xab81);
static_assert(offsetof(DataSegment, recon_refusal_1) == 0xab90);
static_assert(offsetof(DataSegment, recon_refusal_2) == 0xabb6);
static_assert(offsetof(DataSegment, recon_refusal_3) == 0xabe6);
static_assert(offsetof(DataSegment, recon_hour) == 0xac0d);
static_assert(offsetof(DataSegment, recon_hours) == 0xac12);
static_assert(offsetof(DataSegment, recon_duration) == 0xac18);
static_assert(offsetof(DataSegment, recon_lieutenant) == 0xac43);
static_assert(offsetof(DataSegment, text_ac68) == 0xac68);
static_assert(offsetof(DataSegment, text_ac6e) == 0xac6e);
static_assert(offsetof(DataSegment, text_ac78) == 0xac78);
static_assert(offsetof(DataSegment, text_ac81) == 0xac81);
static_assert(offsetof(DataSegment, text_ac86) == 0xac86);
static_assert(offsetof(DataSegment, text_ac92) == 0xac92);
static_assert(offsetof(DataSegment, text_ac9c) == 0xac9c);
static_assert(offsetof(DataSegment, text_aca6) == 0xaca6);
static_assert(offsetof(DataSegment, text_acb2) == 0xacb2);
static_assert(offsetof(DataSegment, text_acc0) == 0xacc0);
static_assert(offsetof(DataSegment, text_acce) == 0xacce);
static_assert(offsetof(DataSegment, text_acdd) == 0xacdd);
static_assert(offsetof(DataSegment, text_ace8) == 0xace8);
static_assert(offsetof(DataSegment, special_forces_title) == 0xacf3);
static_assert(offsetof(DataSegment, special_orders_line) == 0xad02);
static_assert(offsetof(DataSegment, special_target_line) == 0xad1c);
static_assert(offsetof(DataSegment, special_time_line) == 0xad36);
static_assert(offsetof(DataSegment, special_cancel_question) == 0xad50);
static_assert(offsetof(DataSegment, special_select_question) == 0xad9a);
static_assert(offsetof(DataSegment, unknown_adb5) == 0xadb5);
static_assert(offsetof(DataSegment, special_missions) == 0xadb6);
static_assert(offsetof(DataSegment, special_menu) == 0xadca);
static_assert(offsetof(DataSegment, near_add2) == 0xadd2);
static_assert(offsetof(DataSegment, bytes_add4) == 0xadd4);
static_assert(offsetof(DataSegment, near_adde) == 0xadde);
static_assert(offsetof(DataSegment, bytes_ade0) == 0xade0);
static_assert(offsetof(DataSegment, near_adea) == 0xadea);
static_assert(offsetof(DataSegment, bytes_adec) == 0xadec);
static_assert(offsetof(DataSegment, near_adf6) == 0xadf6);
static_assert(offsetof(DataSegment, bytes_adf8) == 0xadf8);
static_assert(offsetof(DataSegment, near_ae02) == 0xae02);
static_assert(offsetof(DataSegment, bytes_ae04) == 0xae04);
static_assert(offsetof(DataSegment, special_mission_names) == 0xae12);
static_assert(offsetof(DataSegment, unknown_ae22) == 0xae22);
static_assert(offsetof(DataSegment, settings_data) == 0xae2e);
static_assert(offsetof(DataSegment, settings_aerial) == 0xae2e);
static_assert(offsetof(DataSegment, settings_field) == 0xae35);
static_assert(offsetof(DataSegment, settings_pictures_on) == 0xae3b);
static_assert(offsetof(DataSegment, settings_pictures_off) == 0xae3e);
static_assert(offsetof(DataSegment, settings_battles_view) == 0xae42);
static_assert(offsetof(DataSegment, settings_battles_no) == 0xae47);
static_assert(offsetof(DataSegment, settings_battles_select) == 0xae4a);
static_assert(offsetof(DataSegment, settings_music_on) == 0xae51);
static_assert(offsetof(DataSegment, settings_music_off) == 0xae54);
static_assert(offsetof(DataSegment, settings_summary) == 0xae58);
static_assert(offsetof(DataSegment, battles_label_view) == 0xaecb);
static_assert(offsetof(DataSegment, battles_label_no_view) == 0xaed0);
static_assert(offsetof(DataSegment, battles_label_select) == 0xaed8);
static_assert(offsetof(DataSegment, battles_view_all) == 0xaedf);
static_assert(offsetof(DataSegment, battles_view_none) == 0xaef0);
static_assert(offsetof(DataSegment, battles_view_selected) == 0xaf00);
static_assert(offsetof(DataSegment, battles_question) == 0xaf17);
static_assert(offsetof(DataSegment, battles_reply) == 0xaf32);
static_assert(offsetof(DataSegment, settings_padding) == 0xaf3f);
static_assert(offsetof(DataSegment, battles_menu) == 0xaf40);
static_assert(offsetof(DataSegment, battles_descriptions) == 0xaf70);
static_assert(offsetof(DataSegment, delay_question) == 0xaf76);
static_assert(offsetof(DataSegment, delay_reply) == 0xafa1);
static_assert(offsetof(DataSegment, map_aerial) == 0xafc6);
static_assert(offsetof(DataSegment, map_field) == 0xafcd);
static_assert(offsetof(DataSegment, map_reply) == 0xafd3);
static_assert(offsetof(DataSegment, sound_on) == 0xafef);
static_assert(offsetof(DataSegment, sound_off) == 0xaff2);
static_assert(offsetof(DataSegment, sound_reply) == 0xaff6);
static_assert(offsetof(DataSegment, pictures_will) == 0xb014);
static_assert(offsetof(DataSegment, pictures_will_not) == 0xb019);
static_assert(offsetof(DataSegment, pictures_reply) == 0xb022);
static_assert(offsetof(DataSegment, create_saving) == 0xb04b);
static_assert(offsetof(DataSegment, create_file_name) == 0xb055);
static_assert(offsetof(DataSegment, create_check_name) == 0xb05e);
static_assert(offsetof(DataSegment, create_saved) == 0xb067);
static_assert(offsetof(DataSegment, save_check_name) == 0xb073);
static_assert(offsetof(DataSegment, save_recheck_name) == 0xb07c);
static_assert(offsetof(DataSegment, save_failed) == 0xb085);
static_assert(offsetof(DataSegment, save_failed_format) == 0xb09a);
static_assert(offsetof(DataSegment, save_open_name) == 0xb09d);
static_assert(offsetof(DataSegment, save_where) == 0xb0a6);
static_assert(offsetof(DataSegment, save_confirm) == 0xb0b2);
static_assert(offsetof(DataSegment, save_answer) == 0xb0b9);
static_assert(offsetof(DataSegment, save_saving) == 0xb0ba);
static_assert(offsetof(DataSegment, save_finished) == 0xb0c4);
static_assert(offsetof(DataSegment, logo_file_name) == 0xb0d5);
static_assert(offsetof(DataSegment, leave_question) == 0xb0e2);
static_assert(offsetof(DataSegment, new_game_question) == 0xb0ee);
static_assert(offsetof(DataSegment, save_quit_label_save) == 0xb100);
static_assert(offsetof(DataSegment, save_quit_label_load) == 0xb10a);
static_assert(offsetof(DataSegment, save_quit_label_end) == 0xb118);
static_assert(offsetof(DataSegment, save_quit_question) == 0xb121);
static_assert(offsetof(DataSegment, save_quit_actions) == 0xb132);
static_assert(offsetof(DataSegment, save_quit_menu) == 0xb13e);
static_assert(offsetof(DataSegment, settings_label_battles) == 0xb16e);
static_assert(offsetof(DataSegment, settings_label_delay) == 0xb17c);
static_assert(offsetof(DataSegment, settings_label_maps) == 0xb18a);
static_assert(offsetof(DataSegment, settings_label_music) == 0xb196);
static_assert(offsetof(DataSegment, settings_label_pictures) == 0xb1a0);
static_assert(offsetof(DataSegment, settings_label_save_quit) == 0xb1af);
static_assert(offsetof(DataSegment, settings_title) == 0xb1bb);
static_assert(offsetof(DataSegment, settings_question) == 0xb1c4);
static_assert(offsetof(DataSegment, settings_padding_b1d5) == 0xb1d5);
static_assert(offsetof(DataSegment, settings_actions) == 0xb1d6);
static_assert(offsetof(DataSegment, settings_menu) == 0xb1ee);
static_assert(offsetof(DataSegment, combat_text) == 0xb242);
static_assert(offsetof(DataSegment, text_cannot_move) == 0xb242);
static_assert(offsetof(DataSegment, text_cannot_fight) == 0xb24f);
static_assert(offsetof(DataSegment, text_defenders) == 0xb25d);
static_assert(offsetof(DataSegment, supply_notice_format) == 0xb267);
static_assert(offsetof(DataSegment, battle_siege_stalemate_text) == 0xb287);
static_assert(offsetof(DataSegment, battle_victory_format) == 0xb2a0);
static_assert(offsetof(DataSegment, battle_stalemate_text) == 0xb2bb);
static_assert(offsetof(DataSegment, btlready_data) == 0xb2d4);
static_assert(offsetof(DataSegment, battle_ready_blank) == 0xb2d4);
static_assert(offsetof(DataSegment, unknown_b2d5) == 0xb2d5);
static_assert(offsetof(DataSegment, battle_ready_name_format) == 0xb2d6);
static_assert(offsetof(DataSegment, battle_ready_text) == 0xb2e3);
static_assert(offsetof(DataSegment, battle_round_format) == 0xb2fe);
static_assert(offsetof(DataSegment, btlcalc_data) == 0xb312);
static_assert(offsetof(DataSegment, btlcalc_skill_class) == 0xb312);
static_assert(offsetof(DataSegment, btlcalc_terrain_class) == 0xb31a);
static_assert(offsetof(DataSegment, btlcalc_step_x) == 0xb346);
static_assert(offsetof(DataSegment, btlcalc_step_y) == 0xb34a);
static_assert(offsetof(DataSegment, battle_text) == 0xb34e);
static_assert(offsetof(DataSegment, battle_fmt_garrison) == 0xb34e);
static_assert(offsetof(DataSegment, battle_fmt_title) == 0xb364);
static_assert(offsetof(DataSegment, battle_fmt_ok) == 0xb375);
static_assert(offsetof(DataSegment, battle_fmt_damage) == 0xb37f);
static_assert(offsetof(DataSegment, battle_fmt_morale) == 0xb389);
static_assert(offsetof(DataSegment, battle_fmt_training) == 0xb393);
static_assert(offsetof(DataSegment, battle_fmt_fatigue) == 0xb39d);
static_assert(offsetof(DataSegment, battle_fmt_fuel) == 0xb3a7);
static_assert(offsetof(DataSegment, battle_fmt_ammo) == 0xb3b0);
static_assert(offsetof(DataSegment, battle_fmt_no_orders) == 0xb3b9);
static_assert(offsetof(DataSegment, battle_fmt_orders) == 0xb3c6);
static_assert(offsetof(DataSegment, battle_fmt_target) == 0xb3d3);
static_assert(offsetof(DataSegment, battle_fmt_apat) == 0xb3dd);
static_assert(offsetof(DataSegment, battle_fmt_defense) == 0xb3eb);
static_assert(offsetof(DataSegment, battle_fmt_speed) == 0xb3f5);
static_assert(offsetof(DataSegment, battle_fmt_range) == 0xb3ff);
static_assert(offsetof(DataSegment, battle_fmt_end) == 0xb409);
static_assert(offsetof(DataSegment, unknown_b40d) == 0xb40d);
static_assert(offsetof(DataSegment, battle_handlers) == 0xb40e);
static_assert(offsetof(DataSegment, far_code_b412) == 0xb412);
static_assert(offsetof(DataSegment, far_code_b416) == 0xb416);
static_assert(offsetof(DataSegment, far_code_b41a) == 0xb41a);
static_assert(offsetof(DataSegment, far_code_b41e) == 0xb41e);
static_assert(offsetof(DataSegment, far_code_b422) == 0xb422);
static_assert(offsetof(DataSegment, far_code_b426) == 0xb426);
static_assert(offsetof(DataSegment, far_code_b42a) == 0xb42a);
static_assert(offsetof(DataSegment, far_code_b42e) == 0xb42e);
static_assert(offsetof(DataSegment, far_code_b432) == 0xb432);
static_assert(offsetof(DataSegment, sprite_variant) == 0xb436);
static_assert(offsetof(DataSegment, battle_info_widths) == 0xb438);
static_assert(offsetof(DataSegment, battle_top_widths) == 0xb43c);
static_assert(offsetof(DataSegment, battle_mid_left_widths) == 0xb448);
static_assert(offsetof(DataSegment, battle_mid_right_widths) == 0xb44e);
static_assert(offsetof(DataSegment, battle_left_mark) == 0xb454);
static_assert(offsetof(DataSegment, battle_low_left_widths) == 0xb458);
static_assert(offsetof(DataSegment, battle_right_mark) == 0xb45d);
static_assert(offsetof(DataSegment, battle_low_right_widths) == 0xb462);
static_assert(offsetof(DataSegment, battle_commander_format) == 0xb46e);
static_assert(offsetof(DataSegment, battle_division_format) == 0xb474);
static_assert(offsetof(DataSegment, battle_wait_format) == 0xb476);
static_assert(offsetof(DataSegment, battle_prompt_format) == 0xb483);
static_assert(offsetof(DataSegment, battle_fmt_sighted) == 0xb490);
static_assert(offsetof(DataSegment, battle_text_sighted) == 0xb49b);
static_assert(offsetof(DataSegment, hexmap_offsets) == 0xb4aa);
static_assert(offsetof(DataSegment, hex_terrain_codes) == 0xb4b6);
static_assert(offsetof(DataSegment, battle_hurricane_text) == 0xb4e1);
static_assert(offsetof(DataSegment, battle_title_style) == 0xb4eb);
static_assert(offsetof(DataSegment, battle_title_format) == 0xb4ef);
static_assert(offsetof(DataSegment, battle_place_format) == 0xb50a);
static_assert(offsetof(DataSegment, battle_terrain_format) == 0xb51d);
static_assert(offsetof(DataSegment, battle_title_end) == 0xb530);
static_assert(offsetof(DataSegment, unknown_b537) == 0xb537);
static_assert(offsetof(DataSegment, deploy_window) == 0xb540);
static_assert(offsetof(DataSegment, deploy_header) == 0xb553);
static_assert(offsetof(DataSegment, deploy_mark) == 0xb571);
static_assert(offsetof(DataSegment, deploy_name) == 0xb575);
static_assert(offsetof(DataSegment, deploy_placed) == 0xb582);
static_assert(offsetof(DataSegment, deploy_waiting) == 0xb5a3);
static_assert(offsetof(DataSegment, deploy_prompt) == 0xb5c2);
static_assert(offsetof(DataSegment, deploy_refused) == 0xb5e9);
static_assert(offsetof(DataSegment, unknown_b60a) == 0xb60a);
static_assert(offsetof(DataSegment, deploy_title) == 0xb612);
static_assert(offsetof(DataSegment, deploy_unit_title) == 0xb62a);
static_assert(offsetof(DataSegment, deploy_garrison_title) == 0xb638);
static_assert(offsetof(DataSegment, deploy_all) == 0xb64a);
static_assert(offsetof(DataSegment, deploy_others) == 0xb654);
static_assert(offsetof(DataSegment, deploy_ask_auto) == 0xb664);
static_assert(offsetof(DataSegment, deploy_ask_done) == 0xb68c);
static_assert(offsetof(DataSegment, deploy_templates) == 0xb6ac);
static_assert(offsetof(DataSegment, deploy_zone_codes) == 0xb770);
static_assert(offsetof(DataSegment, deploy_zone_bits) == 0xb790);
static_assert(offsetof(DataSegment, deploy_scan) == 0xb854);
static_assert(offsetof(DataSegment, deploy_offsets) == 0xb874);
static_assert(offsetof(DataSegment, hq_fmt_side) == 0xb89c);
static_assert(offsetof(DataSegment, hq_city_garrison) == 0xb8a4);
static_assert(offsetof(DataSegment, hq_new_title) == 0xb8b2);
static_assert(offsetof(DataSegment, hq_new_prompt) == 0xb8c3);
static_assert(offsetof(DataSegment, hq_fmt_moved) == 0xb8de);
static_assert(offsetof(DataSegment, hq_div_killed) == 0xb907);
static_assert(offsetof(DataSegment, hq_div_injured) == 0xb912);
static_assert(offsetof(DataSegment, hq_fmt_division) == 0xb91e);
static_assert(offsetof(DataSegment, hq_army_killed) == 0xb945);
static_assert(offsetof(DataSegment, hq_army_injured) == 0xb950);
static_assert(offsetof(DataSegment, hq_fmt_army) == 0xb95c);
static_assert(offsetof(DataSegment, hq_intel_killed) == 0xb97c);
static_assert(offsetof(DataSegment, hq_intel_injured) == 0xb987);
static_assert(offsetof(DataSegment, hq_fmt_intel) == 0xb993);
static_assert(offsetof(DataSegment, hq_ops_killed) == 0xb9b0);
static_assert(offsetof(DataSegment, hq_ops_injured) == 0xb9bb);
static_assert(offsetof(DataSegment, hq_fmt_ops) == 0xb9c7);
static_assert(offsetof(DataSegment, battle_ai_move_class) == 0xb9e2);
static_assert(offsetof(DataSegment, battle_ai_kind) == 0xb9ea);
static_assert(offsetof(DataSegment, battle_ai_offsets) == 0xb9f2);
static_assert(offsetof(DataSegment, battle_ai_actions) == 0xba1a);
static_assert(offsetof(DataSegment, battle_ai_pairs) == 0xba2e);
static_assert(offsetof(DataSegment, battle_fmt_wiped) == 0xba3e);
static_assert(offsetof(DataSegment, battle_text_vs) == 0xba53);
static_assert(offsetof(DataSegment, battle_text_s) == 0xba56);
static_assert(offsetof(DataSegment, battle_text_bunker) == 0xba59);
static_assert(offsetof(DataSegment, battle_fmt_firing0) == 0xba60);
static_assert(offsetof(DataSegment, battle_fmt_firing1) == 0xba74);
static_assert(offsetof(DataSegment, battle_skill_kinds) == 0xba88);
static_assert(offsetof(DataSegment, mine_text) == 0xba90);
static_assert(offsetof(DataSegment, retreat_mines_text) == 0xbaa2);
static_assert(offsetof(DataSegment, unknown_bacb) == 0xbacb);
static_assert(offsetof(DataSegment, battle_cursor_x) == 0xbacc);
static_assert(offsetof(DataSegment, battle_cursor_y) == 0xbacd);
static_assert(offsetof(DataSegment, battle_move_class) == 0xbace);
static_assert(offsetof(DataSegment, battle_fmt_cursor) == 0xbad6);
static_assert(offsetof(DataSegment, battle_fmt_extra) == 0xbae4);
static_assert(offsetof(DataSegment, battle_fmt_cell_ok) == 0xbaea);
static_assert(offsetof(DataSegment, battle_msg_cannot_move) == 0xbaf2);
static_assert(offsetof(DataSegment, battle_msg_select_target) == 0xbb0b);
static_assert(offsetof(DataSegment, battle_msg_set_target) == 0xbb28);
static_assert(offsetof(DataSegment, battle_msg_no_units) == 0xbb45);
static_assert(offsetof(DataSegment, battle_msg_withdraw_from) == 0xbb5f);
static_assert(offsetof(DataSegment, text_bb81) == 0xbb81);
static_assert(offsetof(DataSegment, text_bb86) == 0xbb86);
static_assert(offsetof(DataSegment, text_bb8d) == 0xbb8d);
static_assert(offsetof(DataSegment, text_bb93) == 0xbb93);
static_assert(offsetof(DataSegment, text_bb9a) == 0xbb9a);
static_assert(offsetof(DataSegment, unknown_bba3) == 0xbba3);
static_assert(offsetof(DataSegment, battle_menu_items) == 0xbba4);
static_assert(offsetof(DataSegment, battle_menu_places) == 0xbbec);
static_assert(offsetof(DataSegment, battle_msg_orders) == 0xbc00);
static_assert(offsetof(DataSegment, battle_msg_cannot_withdraw) == 0xbc15);
static_assert(offsetof(DataSegment, battle_fmt_side_orders) == 0xbc2c);
static_assert(offsetof(DataSegment, battle_msg_preparing) == 0xbc4b);
static_assert(offsetof(DataSegment, battle_msg_orders_finished) == 0xbc63);
static_assert(offsetof(DataSegment, unknown_bc7d) == 0xbc7d);
static_assert(offsetof(DataSegment, battle_retreat_class) == 0xbc7e);
static_assert(offsetof(DataSegment, dispatch_error) == 0xbc86);
static_assert(offsetof(DataSegment, message_table) == 0xbc98);
static_assert(offsetof(DataSegment, order_no_fuel) == 0xbeb4);
static_assert(offsetof(DataSegment, order_no_ammo) == 0xbecb);
static_assert(offsetof(DataSegment, order_prepared) == 0xbee8);
static_assert(offsetof(DataSegment, order_ask_attack) == 0xbf09);
static_assert(offsetof(DataSegment, order_begin_attack) == 0xbf2b);
static_assert(offsetof(DataSegment, order_occupied) == 0xbf46);
static_assert(offsetof(DataSegment, order_unit_label) == 0xbf5c);
static_assert(offsetof(DataSegment, order_bridge_destroyed) == 0xbf69);
static_assert(offsetof(DataSegment, mines_laid_text) == 0xbf88);
static_assert(offsetof(DataSegment, mines_removed_text) == 0xbfa9);
static_assert(offsetof(DataSegment, bridge_built_text) == 0xbfc7);
static_assert(offsetof(DataSegment, arms_repaired_text) == 0xbfe4);
static_assert(offsetof(DataSegment, unit_arms_repaired_text) == 0xc005);
static_assert(offsetof(DataSegment, transfer_panel_x) == 0xc026);
static_assert(offsetof(DataSegment, transfer_panel_y) == 0xc028);
static_assert(offsetof(DataSegment, transfer_title) == 0xc02a);
static_assert(offsetof(DataSegment, transfer_name_format) == 0xc033);
static_assert(offsetof(DataSegment, transfer_name_text) == 0xc042);
static_assert(offsetof(DataSegment, text_c04e) == 0xc04e);
static_assert(offsetof(DataSegment, text_c051) == 0xc051);
static_assert(offsetof(DataSegment, text_c054) == 0xc054);
static_assert(offsetof(DataSegment, transfer_amount_format) == 0xc057);
static_assert(offsetof(DataSegment, transfer_amount_text) == 0xc061);
static_assert(offsetof(DataSegment, transfer_colour) == 0xc065);
static_assert(offsetof(DataSegment, near_c06c) == 0xc06c);
static_assert(offsetof(DataSegment, near_c06e) == 0xc06e);
static_assert(offsetof(DataSegment, near_c070) == 0xc070);
static_assert(offsetof(DataSegment, transfer_unlimited) == 0xc072);
static_assert(offsetof(DataSegment, transfer_prompt) == 0xc074);
static_assert(offsetof(DataSegment, transfer_begin) == 0xc090);
static_assert(offsetof(DataSegment, unknown_c0ab) == 0xc0ab);
static_assert(offsetof(DataSegment, arrived_text) == 0xc0ac);
static_assert(offsetof(DataSegment, unknown_c0c7) == 0xc0c7);
static_assert(offsetof(DataSegment, order_handlers) == 0xc0c8);
static_assert(offsetof(DataSegment, order_moves) == 0xc100);
static_assert(offsetof(DataSegment, move_class) == 0xc11c);
static_assert(offsetof(DataSegment, sight_ranges) == 0xc124);
static_assert(offsetof(DataSegment, mines_found_text) == 0xc12a);
static_assert(offsetof(DataSegment, minefield_text) == 0xc141);
static_assert(offsetof(DataSegment, lost_sight_format) == 0xc166);
static_assert(offsetof(DataSegment, unknown_c189) == 0xc189);
static_assert(offsetof(DataSegment, notice_command_text) == 0xc18a);
static_assert(offsetof(DataSegment, notice_control_text) == 0xc1b8);
static_assert(offsetof(DataSegment, notice_battle_text) == 0xc1e6);
static_assert(offsetof(DataSegment, notice_attack_format) == 0xc20d);
static_assert(offsetof(DataSegment, notice_under_attack_text) == 0xc22a);
static_assert(offsetof(DataSegment, unknown_c24f) == 0xc24f);
static_assert(offsetof(DataSegment, notend_data) == 0xc250);
static_assert(offsetof(DataSegment, notend_occupy_class) == 0xc250);
static_assert(offsetof(DataSegment, end_occupied_format) == 0xc258);
static_assert(offsetof(DataSegment, end_garrison_name) == 0xc26b);
static_assert(offsetof(DataSegment, end_loss_format) == 0xc274);
static_assert(offsetof(DataSegment, end_wiped_out_text) == 0xc285);
static_assert(offsetof(DataSegment, end_undamaged_text) == 0xc296);
static_assert(offsetof(DataSegment, end_one_loss_format) == 0xc2a9);
static_assert(offsetof(DataSegment, end_losses_format) == 0xc2bd);
static_assert(offsetof(DataSegment, unknown_c2d3) == 0xc2d3);
static_assert(offsetof(DataSegment, atopsp) == 0xc2d4);
static_assert(offsetof(DataSegment, aexit_rtn) == 0xc2d6);
static_assert(offsetof(DataSegment, asizds) == 0xc2d8);
static_assert(offsetof(DataSegment, nheap_desc) == 0xc2da);
static_assert(offsetof(DataSegment, aseghi) == 0xc2ee);
static_assert(offsetof(DataSegment, aseglo) == 0xc2f0);
static_assert(offsetof(DataSegment, acfinfo) == 0xc2f2);
static_assert(offsetof(DataSegment, aintdiv) == 0xc300);
static_assert(offsetof(DataSegment, fac) == 0xc304);
static_assert(offsetof(DataSegment, crt_errno) == 0xc30e);
static_assert(offsetof(DataSegment, umaskval) == 0xc310);
static_assert(offsetof(DataSegment, pspadr) == 0xc312);
static_assert(offsetof(DataSegment, psp) == 0xc314);
static_assert(offsetof(DataSegment, osversion) == 0xc316);
static_assert(offsetof(DataSegment, dosvermajor) == 0xc316);
static_assert(offsetof(DataSegment, osmajor) == 0xc316);
static_assert(offsetof(DataSegment, dosverminor) == 0xc317);
static_assert(offsetof(DataSegment, osminor) == 0xc317);
static_assert(offsetof(DataSegment, osmode) == 0xc318);
static_assert(offsetof(DataSegment, oserr) == 0xc319);
static_assert(offsetof(DataSegment, doserrno) == 0xc319);
static_assert(offsetof(DataSegment, nfile) == 0xc31b);
static_assert(offsetof(DataSegment, osfile) == 0xc31d);
static_assert(offsetof(DataSegment, argc) == 0xc331);
static_assert(offsetof(DataSegment, argv) == 0xc333);
static_assert(offsetof(DataSegment, crt_environ) == 0xc335);
static_assert(offsetof(DataSegment, pgmptr) == 0xc337);
static_assert(offsetof(DataSegment, child) == 0xc33c);
static_assert(offsetof(DataSegment, ovlflag) == 0xc33e);
static_assert(offsetof(DataSegment, intno) == 0xc33f);
static_assert(offsetof(DataSegment, ovlvec) == 0xc340);
static_assert(offsetof(DataSegment, chbuf) == 0xc344);
static_assert(offsetof(DataSegment, adbgmsg) == 0xc346);
static_assert(offsetof(DataSegment, qczrinit) == 0xc35e);
static_assert(offsetof(DataSegment, a_dbswpflg) == 0xc360);
static_assert(offsetof(DataSegment, a_dbswpchk) == 0xc362);
static_assert(offsetof(DataSegment, a_dbrterr) == 0xc364);
static_assert(offsetof(DataSegment, a_dbexit) == 0xc366);
static_assert(offsetof(DataSegment, a_dbptrchk) == 0xc36c);
static_assert(offsetof(DataSegment, fpinit) == 0xc370);
static_assert(offsetof(DataSegment, toward_segment) == 0xc37c);
static_assert(offsetof(DataSegment, unknown_c37e) == 0xc37e);
static_assert(offsetof(DataSegment, unitai_segment) == 0xc380);
static_assert(offsetof(DataSegment, btlcurs_segment) == 0xc382);
static_assert(offsetof(DataSegment, movecost_segment) == 0xc384);
static_assert(offsetof(DataSegment, unknown_c386) == 0xc386);
static_assert(offsetof(DataSegment, edata) == 0xc46a);
static_assert(offsetof(DataSegment, tile_ems_handle) == 0xc46a);
static_assert(offsetof(DataSegment, ems_pages) == 0xc46c);
static_assert(offsetof(DataSegment, frame_ems_handle) == 0xc47c);
static_assert(offsetof(DataSegment, sprite_ems_handle) == 0xc47e);
static_assert(offsetof(DataSegment, numdlg_ems_handle) == 0xc480);
static_assert(offsetof(DataSegment, people_backup) == 0xc482);
static_assert(offsetof(DataSegment, scenario_text_x) == 0xc6ee);
static_assert(offsetof(DataSegment, scenario_text_y) == 0xc6f0);
static_assert(offsetof(DataSegment, end_score) == 0xc6f2);
static_assert(offsetof(DataSegment, random_state) == 0xc6f4);
static_assert(offsetof(DataSegment, list_mouse) == 0xc742);
static_assert(offsetof(DataSegment, tbl_save_map) == 0xc744);
static_assert(offsetof(DataSegment, tbl_save_level) == 0xc746);
static_assert(offsetof(DataSegment, initial_person) == 0xc748);
static_assert(offsetof(DataSegment, initial_template5) == 0xc74a);
static_assert(offsetof(DataSegment, initial_template4) == 0xc74c);
static_assert(offsetof(DataSegment, brief_mark_x) == 0xc74e);
static_assert(offsetof(DataSegment, brief_mark_y) == 0xc750);
static_assert(offsetof(DataSegment, daily_record) == 0xc752);
static_assert(offsetof(DataSegment, daily_side) == 0xc754);
static_assert(offsetof(DataSegment, unknown_c755) == 0xc755);
static_assert(offsetof(DataSegment, air_side) == 0xc756);
static_assert(offsetof(DataSegment, unknown_c757) == 0xc757);
static_assert(offsetof(DataSegment, event_side) == 0xc758);
static_assert(offsetof(DataSegment, unknown_c759) == 0xc759);
static_assert(offsetof(DataSegment, arrival_unit) == 0xc75a);
static_assert(offsetof(DataSegment, pending_request) == 0xc75c);
static_assert(offsetof(DataSegment, request_x) == 0xc75e);
static_assert(offsetof(DataSegment, request_y) == 0xc75f);
static_assert(offsetof(DataSegment, request_error) == 0xc760);
static_assert(offsetof(DataSegment, unknown_c761) == 0xc761);
static_assert(offsetof(DataSegment, request_person) == 0xc762);
static_assert(offsetof(DataSegment, request_template) == 0xc764);
static_assert(offsetof(DataSegment, arrival_side) == 0xc766);
static_assert(offsetof(DataSegment, unknown_c767) == 0xc767);
static_assert(offsetof(DataSegment, invalid_request) == 0xc768);
static_assert(offsetof(DataSegment, flash_buffers) == 0xc76a);
static_assert(offsetof(DataSegment, transfer_unit) == 0xc86a);
static_assert(offsetof(DataSegment, transfer_group) == 0xc86b);
static_assert(offsetof(DataSegment, advice_messages) == 0xc86c);
static_assert(offsetof(DataSegment, supply_objective) == 0xca60);
static_assert(offsetof(DataSegment, supply_objective_bytes) == 0xca60);
static_assert(offsetof(DataSegment, supply_table) == 0xca62);
static_assert(offsetof(DataSegment, supply_total) == 0xca86);
static_assert(offsetof(DataSegment, supply_count) == 0xca88);
static_assert(offsetof(DataSegment, unknown_ca89) == 0xca89);
static_assert(offsetof(DataSegment, supply_amounts) == 0xca8a);
static_assert(offsetof(DataSegment, tally_first_name) == 0xcaa2);
static_assert(offsetof(DataSegment, tally_second_name) == 0xcac1);
static_assert(offsetof(DataSegment, supply_types) == 0xcae0);
static_assert(offsetof(DataSegment, tally_reject_count) == 0xcaec);
static_assert(offsetof(DataSegment, unknown_caed) == 0xcaed);
static_assert(offsetof(DataSegment, supply_others) == 0xcaee);
static_assert(offsetof(DataSegment, supply_sum_b) == 0xcb06);
static_assert(offsetof(DataSegment, supply_sum_a) == 0xcb36);
static_assert(offsetof(DataSegment, supply_sum_c) == 0xcb66);
static_assert(offsetof(DataSegment, supply_avg_b) == 0xcb96);
static_assert(offsetof(DataSegment, supply_avg_a) == 0xcba2);
static_assert(offsetof(DataSegment, supply_avg_c) == 0xcbae);
static_assert(offsetof(DataSegment, tally_unit) == 0xcbba);
static_assert(offsetof(DataSegment, alloc_units) == 0xcbba);
static_assert(offsetof(DataSegment, tally_other_unit) == 0xcbbc);
static_assert(offsetof(DataSegment, tally_source) == 0xcbbe);
static_assert(offsetof(DataSegment, alloc_objective) == 0xcbbe);
static_assert(offsetof(DataSegment, supply_spare) == 0xcbc0);
static_assert(offsetof(DataSegment, alloc_slots) == 0xcbde);
static_assert(offsetof(DataSegment, rq_type_list) == 0xcc1a);
static_assert(offsetof(DataSegment, air_stat_14) == 0xccb8);
static_assert(offsetof(DataSegment, air_stat_1b) == 0xccb9);
static_assert(offsetof(DataSegment, air_stat_13) == 0xccba);
static_assert(offsetof(DataSegment, unknown_ccbb) == 0xccbb);
static_assert(offsetof(DataSegment, air_person) == 0xccbc);
static_assert(offsetof(DataSegment, air_troops) == 0xccbe);
static_assert(offsetof(DataSegment, special_charm) == 0xccc0);
static_assert(offsetof(DataSegment, special_bravery) == 0xccc1);
static_assert(offsetof(DataSegment, special_person) == 0xccc2);
static_assert(offsetof(DataSegment, special_order) == 0xccc4);
static_assert(offsetof(DataSegment, special_experience) == 0xccc6);
static_assert(offsetof(DataSegment, special_leadership) == 0xccc7);
static_assert(offsetof(DataSegment, vram) == 0xccc8);
static_assert(offsetof(DataSegment, transfer_auto) == 0xced0);
static_assert(offsetof(DataSegment, unknown_ced1) == 0xced1);
static_assert(offsetof(DataSegment, transfer_objective_ptr) == 0xced2);
static_assert(offsetof(DataSegment, transfer_objective_ptr_bytes) == 0xced2);
static_assert(offsetof(DataSegment, transfer_table) == 0xced4);
static_assert(offsetof(DataSegment, transfer_interactive) == 0xcef8);
static_assert(offsetof(DataSegment, unknown_cefa) == 0xcefa);
static_assert(offsetof(DataSegment, battle_terrain) == 0xcf00);
static_assert(offsetof(DataSegment, ai_unit_list) == 0xcfc6);
static_assert(offsetof(DataSegment, supply_units) == 0xcfc6);
static_assert(offsetof(DataSegment, save_slots) == 0xd006);
static_assert(offsetof(DataSegment, icon_segment) == 0xd0f6);
static_assert(offsetof(DataSegment, battle_positions) == 0xd0f8);
static_assert(offsetof(DataSegment, tbl_objective_list) == 0xd350);
static_assert(offsetof(DataSegment, view_objectives) == 0xd350);
static_assert(offsetof(DataSegment, map_tiles) == 0xd37a);
static_assert(offsetof(DataSegment, strength_items) == 0xd9ba);
static_assert(offsetof(DataSegment, view_stocks) == 0xd9ba);
static_assert(offsetof(DataSegment, arrival_candidates) == 0xd9c6);
static_assert(offsetof(DataSegment, bitmap_buffer) == 0xe006);
static_assert(offsetof(DataSegment, image_segment) == 0xe006);
static_assert(offsetof(DataSegment, battle_losses) == 0xe008);
static_assert(offsetof(DataSegment, sound_driver) == 0xe224);
static_assert(offsetof(DataSegment, event_buffer) == 0xe226);
static_assert(offsetof(DataSegment, request_buffer) == 0xe28a);
static_assert(offsetof(DataSegment, supply_window) == 0xe2ee);
static_assert(offsetof(DataSegment, list_window) == 0xe2ee);
static_assert(offsetof(DataSegment, list_column) == 0xe2f0);
static_assert(offsetof(DataSegment, supply_top) == 0xe2f1);
static_assert(offsetof(DataSegment, list_top) == 0xe2f1);
static_assert(offsetof(DataSegment, list_columns) == 0xe2f2);
static_assert(offsetof(DataSegment, list_rows) == 0xe2f3);
static_assert(offsetof(DataSegment, save_slot) == 0xe2f4);
static_assert(offsetof(DataSegment, supply_row) == 0xe2f4);
static_assert(offsetof(DataSegment, list_row) == 0xe2f4);
static_assert(offsetof(DataSegment, unknown_e2f5) == 0xe2f5);
static_assert(offsetof(DataSegment, scenario_brief) == 0xe2f6);
static_assert(offsetof(DataSegment, scenario_notes) == 0xe3be);
static_assert(offsetof(DataSegment, message_picture_segment) == 0xe616);
static_assert(offsetof(DataSegment, numdlg_image_segment) == 0xe616);
static_assert(offsetof(DataSegment, hexmap_index) == 0xe618);
static_assert(offsetof(DataSegment, unknown_e619) == 0xe619);
static_assert(offsetof(DataSegment, battle_elements) == 0xe61a);
static_assert(offsetof(DataSegment, message_buffer) == 0xe668);
static_assert(offsetof(DataSegment, overview_tile_segment) == 0xe6fe);
static_assert(offsetof(DataSegment, stream_ptr) == 0xe700);
static_assert(offsetof(DataSegment, stream_ptr_bytes) == 0xe700);
static_assert(offsetof(DataSegment, gfx_source) == 0xe700);
static_assert(offsetof(DataSegment, tbl_type_list) == 0xe704);
static_assert(offsetof(DataSegment, weapon_list) == 0xe704);
static_assert(offsetof(DataSegment, hex_display_segment) == 0xe840);
static_assert(offsetof(DataSegment, vseg_segment) == 0xe842);
static_assert(offsetof(DataSegment, numdlg_save_segment) == 0xe844);
static_assert(offsetof(DataSegment, ai_element_list) == 0xe846);
static_assert(offsetof(DataSegment, tally_records) == 0xe846);
static_assert(offsetof(DataSegment, alloc_elements) == 0xe846);
static_assert(offsetof(DataSegment, battle_garrison) == 0xe946);
static_assert(offsetof(DataSegment, garrison_unit) == 0xe946);
static_assert(offsetof(DataSegment, frame_segment) == 0xe95c);
static_assert(offsetof(DataSegment, battle_path_queue) == 0xe95e);
static_assert(offsetof(DataSegment, unknown_e95f) == 0xe95f);
static_assert(offsetof(DataSegment, sprite_segment) == 0xea22);
static_assert(offsetof(DataSegment, tile_segment) == 0xea24);
static_assert(offsetof(DataSegment, face_segment) == 0xea26);
static_assert(offsetof(DataSegment, portrait_segment) == 0xea28);
static_assert(offsetof(DataSegment, general_list) == 0xea2a);
static_assert(offsetof(DataSegment, battle_path_costs) == 0xea52);
static_assert(offsetof(DataSegment, battle_path_costs_bytes) == 0xea52);
static_assert(offsetof(DataSegment, stack) == 0xebe0);
static_assert(offsetof(DataSegment, end) == 0xebe0);

// Counts and strides of the record tables.
inline constexpr auto SIDE_COUNT{ 2u }; // the two sides
inline constexpr auto GROUPS_PER_SIDE{ 4u };
inline constexpr auto UNITS_PER_GROUP{ 8u };
inline constexpr auto UNITS_PER_SIDE{ 32u };
inline constexpr auto ELEMENTS_PER_SIDE{ 128u };
inline constexpr auto PEOPLE_PER_SIDE{ 20u };
inline constexpr auto OBJECTIVE_COUNT{ 21u };
inline constexpr auto ELEMENT_TYPE_COUNT{ 158u };
inline constexpr auto PERSON_TEMPLATE_COUNT{ 12u };
inline constexpr auto SAVE_SLOT_COUNT{ 20u };
inline constexpr auto MAP_WIDTH{
  40u
}; // map_cells is MAP_WIDTH x MAP_HEIGHT bytes
inline constexpr auto MAP_HEIGHT{ 40u };
inline constexpr auto INFLUENCE_WIDTH{
  10u
}; // influence_map is 10 x 10 signed words
inline constexpr auto BATTLE_WIDTH{ 14u }; // battle_cells is 14 x 14 bytes

// Near offsets of the record tables (what the original code adds an index times a stride to).
inline constexpr std::uint16_t SIDES_AT{ offsetof(DataSegment, sides) };
inline constexpr std::uint16_t PEOPLE_AT{ offsetof(DataSegment, people) };
inline constexpr std::uint16_t GROUPS_AT{ offsetof(DataSegment, groups) };
inline constexpr std::uint16_t ELEMENTS_AT{ offsetof(DataSegment, elements) };
inline constexpr std::uint16_t UNITS_AT{ offsetof(DataSegment, units) };
inline constexpr std::uint16_t OBJECTIVES_AT{ offsetof(DataSegment,
                                                       objectives) };
inline constexpr std::uint16_t ELEMENT_TYPES_AT{ offsetof(DataSegment,
                                                          element_types) };
inline constexpr std::uint16_t DAILY_RECORDS_AT{ offsetof(DataSegment,
                                                          daily_records) };
inline constexpr std::uint16_t REQUEST_CONFIGS_AT{ offsetof(DataSegment,
                                                            request_configs) };
inline constexpr std::uint16_t SPECIAL_AT{ offsetof(DataSegment, special) };
inline constexpr std::uint16_t PERSON_TEMPLATES_AT{ offsetof(
    DataSegment, person_templates) };
inline constexpr std::uint16_t GARRISON_AT{ offsetof(DataSegment, garrison) };
inline constexpr std::uint16_t SAVE_SLOTS_AT{ offsetof(DataSegment,
                                                       save_slots) };

// The segment behind a DS pointer (the build has -fno-strict-aliasing).
[[nodiscard]] inline auto Data(std::uint8_t* ds) noexcept -> DataSegment& {
  return *reinterpret_cast<DataSegment*>(ds);
}
[[nodiscard]] inline auto Data(std::uint8_t const* ds) noexcept
    -> DataSegment const& {
  return *reinterpret_cast<DataSegment const*>(ds);
}

// The record a 16-bit near pointer (a DS offset) addresses, and back.
template <typename T>
[[nodiscard]] inline auto Record(DataSegment& data, std::uint16_t near) noexcept
    -> T& {
  return *reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(&data) + near);
}
template <typename T>
[[nodiscard]] inline auto Record(DataSegment const& data,
                                 std::uint16_t      near) noexcept -> T const& {
  return *reinterpret_cast<T const*>(
      reinterpret_cast<std::uint8_t const*>(&data) + near);
}
template <typename T>
[[nodiscard]] inline auto Near(DataSegment const& data,
                               T const& record) noexcept -> std::uint16_t {
  return static_cast<std::uint16_t>(
      reinterpret_cast<std::uint8_t const*>(&record) -
      reinterpret_cast<std::uint8_t const*>(&data));
}
} // namespace opeu::game::detail::data_segment

namespace opeu::game {
using detail::data_segment::AirDrop;
using detail::data_segment::AirLift;
using detail::data_segment::BombingFlight;
using detail::data_segment::Assignment;
using detail::data_segment::PERSON_VACANT;
using detail::data_segment::RequestKind;
using detail::data_segment::AirStrike;
using detail::data_segment::BATTLE_WIDTH;
using detail::data_segment::BattleView;
using detail::data_segment::Bits;
using detail::data_segment::CacheNode;
using detail::data_segment::Category;
using detail::data_segment::MapCell;
using detail::data_segment::Terrain;
using detail::data_segment::DAILY_RECORDS_AT;
using detail::data_segment::DailyRecord;
using detail::data_segment::Data;
using detail::data_segment::DataSegment;
using detail::data_segment::Element;
using detail::data_segment::ELEMENT_TYPE_COUNT;
using detail::data_segment::ELEMENT_TYPES_AT;
using detail::data_segment::ElementFlag;
using detail::data_segment::ELEMENTS_AT;
using detail::data_segment::ELEMENTS_PER_SIDE;
using detail::data_segment::ElementType;
using detail::data_segment::FarPointer;
using detail::data_segment::Group;
using detail::data_segment::GroupFlag;
using detail::data_segment::GROUPS_AT;
using detail::data_segment::GROUPS_PER_SIDE;
using detail::data_segment::INFLUENCE_WIDTH;
using detail::data_segment::Item;
using detail::data_segment::ListWindow;
using detail::data_segment::TypeFlag;
using detail::data_segment::MAP_HEIGHT;
using detail::data_segment::MAP_WIDTH;
using detail::data_segment::Near;
using detail::data_segment::Objective;
using detail::data_segment::OBJECTIVE_COUNT;
using detail::data_segment::ObjectiveFlag;
using detail::data_segment::OBJECTIVES_AT;
using detail::data_segment::NoonEvent;
using detail::data_segment::Order;
using detail::data_segment::PEOPLE_AT;
using detail::data_segment::PEOPLE_PER_SIDE;
using detail::data_segment::Person;
using detail::data_segment::PERSON_TEMPLATE_COUNT;
using detail::data_segment::PERSON_TEMPLATES_AT;
using detail::data_segment::PersonRole;
using detail::data_segment::StaffRole;
using detail::data_segment::Record;
using detail::data_segment::REQUEST_CONFIGS_AT;
using detail::data_segment::RequestConfig;
using detail::data_segment::RequestSlot;
using detail::data_segment::SAVE_SLOT_COUNT;
using detail::data_segment::SAVE_SLOTS_AT;
using detail::data_segment::GARRISON_AT;
using detail::data_segment::BattleLoss;
using detail::data_segment::SaveSlot;
using detail::data_segment::OrderPrompts;
using detail::data_segment::ScreenPoint;
using detail::data_segment::Side;
using detail::data_segment::ScheduleFlag;
using detail::data_segment::SideFlag;
using detail::data_segment::Stance;
using detail::data_segment::SIDE_PLAYED;
using detail::data_segment::SIDE_COUNT;
using detail::data_segment::SIDES_AT;
using detail::data_segment::SPECIAL_AT;
using detail::data_segment::Task;
using detail::data_segment::TaskKind;
using detail::data_segment::Unit;
using detail::data_segment::UnitFlag;
using detail::data_segment::UnitOrder;
using detail::data_segment::UNITS_AT;
using detail::data_segment::UNITS_PER_GROUP;
using detail::data_segment::UNITS_PER_SIDE;
} // namespace opeu::game
