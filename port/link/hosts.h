/* Struct hosts of the modules whose host entries take a host structure
 * instead of a bare callback. Each owner link file mirrors the module's
 * structure and returns its single instance; callers pass the result
 * straight to that module's routines (their prototypes take `void *`). */
#ifndef PORT_LINK_HOSTS_H
#define PORT_LINK_HOSTS_H

void *link_textout_host(void);   /* core/textout.c TextHost   (port/link/core/textout.c) */
void *link_uiprim_host(void);    /* core/uiprim.c UiHost      (port/link/core/uiprim.c) */
void *link_dialogs_host(void);   /* core/dialogs.c DialogHost (port/link/core/dialogs.c) */
void *link_numdlg_host(void);    /* io/numdlg.c UiHost        (port/link/io/numdlg.c) */

#endif
