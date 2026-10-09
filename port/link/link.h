/* Link layer: connects the service callbacks of the recovered host branches
 * to the recovered routines and to the native services.
 *
 * One file per recovered module: port/link/<area>/<module>.c. It contains
 *
 *   1. the module's service callback(s), named link_<module> (or
 *      link_<module>_<what> when a module takes several), with exactly the
 *      callback type the module declares. Each numbered service performs the
 *      original callee: a recovered routine (called through its host entry
 *      from port/gen/recovered.h, passing that module's own link_ callback)
 *      or a native service (port/services). Unknown ids end in
 *      port_unhandled("<module>", id, count).
 *
 *   2. PORT_ROUTINE(name) handlers for the module's routines that other
 *      modules reach by routine identifier (port_call / port_service), in the
 *      address-keyed word convention of port.h.
 *
 * Rules: no game logic here. A service either calls the recovered routine
 * that the original called or a native replacement of an assembly/DOS/BIOS
 * service. Argument and result conversions (DS offsets, segment:offset pairs,
 * byte/word widths, DX:AX) are the only computation allowed.
 */
#ifndef PORT_LINK_H
#define PORT_LINK_H

#include "port.h"
#include "recovered.h"
#include "services.h"
#include "hosts.h"

/* Argument helpers for the event argument arrays. */
#define ARG(i) ((uint16_t)((i) < count ? args[(i)] : 0))
#define ARG32(i) ((uint32_t)ARG(i) | (uint32_t)ARG((i) + 1) << 16)

/* Call a linked routine by name from a link file: the name is resolved once. */
uint32_t link_call_named(uint8_t *ds, const char *name, const uint16_t *args, unsigned count);

/* Service callbacks defined by the link files (one prototype per module
 * callback; the types are the module's own callback types written out). */
#include "callbacks.h"

#endif
