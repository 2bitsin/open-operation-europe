/* Struct hosts of the three modules whose entries take a host structure.
 * Each getter returns the single instance; its members are platform state the
 * modules read: the arena, the DS scratch area and the segment numbers of the
 * layout. */
#include "link/glue.hpp"

#include "common/game.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/numdlg.hpp"

/* ---- TEXTOUT: TextHost {} ----------------------------------------------------- */
static TextHost textout_host{};

void* port_textout_host(void) {
  return &textout_host;
}

/* ---- UIPRIM: UiHost {memory, ds_segment, tiles_segment} ---------------------- */
namespace {
constexpr std::uint16_t UIPRIM_TILES_SEGMENT{ 0x3873 }; // the far segment of opeu_frame_tiles (data/fg3873)
} // namespace

static UiHost uiprim_host = { 0, PORT_DS_SEGMENT, UIPRIM_TILES_SEGMENT };

void* port_uiprim_host(void) {
  uiprim_host.memory = port_arena;
  return &uiprim_host;
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
