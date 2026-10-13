/* The shipped data files, compiled into the executable: the application
 * (sources/opeu/opeu/assets.cpp) hands services/fileio.c its lookup before
 * the game starts. A registration, not a link edge: the services cannot
 * depend on the application module. */
#pragma once

#include <stddef.h>

/* A DOS file name ("SCEDATA", "c:\\opeu\\msg.16p") -> the shipped bytes;
 * 1 when found, else 0. */
typedef int (*OpeuAssetFind)(const char* dos_name, const unsigned char** data, size_t* size);

void FilesUseAssets(OpeuAssetFind find);
