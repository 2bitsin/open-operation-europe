/* The opening program OPEN.EXE in the native port.
 *
 * The original launcher (KOEI.COM, src/koei/launcher.asm:186-194) sets BIOS
 * mode 12h, runs FMDRV.COM (resident), then OPEN.EXE, then MAIN.EXE. The
 * port does the same in one process: port/main.c installs the FM driver,
 * open_intro_run() runs OPEN's recovered main() (src/open/core/openmain.c)
 * and then MAIN starts with the same window, VGA state and driver.
 *
 *   open_intro_options()  takes --no-intro / --intro out of argv (call
 *                         before the port's own option parser)
 *   open_intro_run()      the opening, unless disabled; afterwards the data
 *                         arena holds MAIN's initial image again
 *
 * The intro runs by default in a window; headless runs (--headless) skip it
 * unless --intro is given, so the existing scripted runs, their PNGs and
 * audio checks keep their timings.
 */
#pragma once

void open_intro_options(int* argc, char** argv);
void open_intro_run(void);
