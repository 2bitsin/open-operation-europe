/* The shipped data files, compiled into the executable: the application
 * (sources/opeu/opeu/assets.cpp) hands services/fileio.c its lookup before
 * the game starts. A registration, not a link edge: the services cannot
 * depend on the application module. */
#ifndef OPEU_SERVICES_ASSET_TABLE_H
#define OPEU_SERVICES_ASSET_TABLE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A DOS file name ("SCEDATA", "c:\\opeu\\msg.16p") -> the shipped bytes;
 * 1 when found, else 0. */
typedef int (*OpeuAssetFind)(const char *dos_name, const unsigned char **data, size_t *size);

void opeu_files_use_assets(OpeuAssetFind find);

#ifdef __cplusplus
}
#endif
#endif
