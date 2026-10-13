/* Struct hosts of the three modules whose entries take a host structure.
 * Each getter returns the single instance; uiprim's `event` is its service
 * (svc_uiprim), the other members are platform state the modules read: the
 * arena, the DS scratch area and the segment numbers of the layout. */
#include "link/glue.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/numdlg.hpp"
#include "link/entry-game.hpp"

/* ---- TEXTOUT: TextHost {} ----------------------------------------------------- */
static TextHost textout_host{};

void* port_textout_host(void) {
  return &textout_host;
}

/* ---- UIPRIM: UiHost {memory, ds_segment, tiles_segment, code_segment, event} - */
namespace {
constexpr std::uint16_t UIPRIM_CODE_SEGMENT{ 0x0a04 };  // UIPRIM's code segment: its far code pointers name it
constexpr std::uint16_t UIPRIM_TILES_SEGMENT{ 0x3873 }; // the far segment of opeu_frame_tiles (data/fg3873)
constexpr std::uint16_t UIPRIM_FILE_READER{ 0x0132 };   // opeu_stream_file_byte in UIPRIM_CODE_SEGMENT
constexpr std::uint16_t UIPRIM_MEMORY_READER{ 0x01fc }; // opeu_stream_memory_byte in UIPRIM_CODE_SEGMENT
} // namespace

static UiHost uiprim_host = { 0, PORT_DS_SEGMENT, UIPRIM_TILES_SEGMENT, UIPRIM_CODE_SEGMENT, svc_uiprim };

void* port_uiprim_host(void) {
  uiprim_host.memory = port_arena;
  return &uiprim_host;
}

/* The decoder's reader is a far code pointer into UIPRIM's own segment:
 * opeu_stream_file_byte or opeu_stream_memory_byte, both called through the
 * same host instance, for the running game (the decoder has none to hand). */
static uint8_t read_file_stream(void) {
  return opeu_stream_file_byte(opeu::game::EntryGame::Current(), (const UiHost*)port_uiprim_host());
}

static uint8_t read_memory_stream(void) {
  return opeu_stream_memory_byte(opeu::game::EntryGame::Current(), (const UiHost*)port_uiprim_host());
}

PortByteReader port_picture_reader(uint16_t off, uint16_t seg) {
  if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_FILE_READER) return read_file_stream;
  if (seg == UIPRIM_CODE_SEGMENT && off == UIPRIM_MEMORY_READER) return read_memory_stream;
  port_fatal("uiprim: picture reader %04x:%04x is not a known stream reader", seg, off);
  return 0;
}

/* ---- DIALOGS: DialogHost {load, scratch} ------------------------------------- *
 * The dialogs' stack buffers (at most 132 bytes, one owner active at a time)
 * take one fixed DS area: the bottom of the 5120-byte stack (PORT_DS_END),
 * which the modeled stack (top PORT_STACK_TOP) never reaches in practice.
 * The load segment is 0. */
static const DialogHost dialogs_host = { 0u, PORT_DS_END };

void* port_dialogs_host(void) {
  return (void*)&dialogs_host;
}
