Init_submodule()
Link_dependencies(opeu-game      # MAIN's STRINGS and MEMORY routines, linked into OPEN.EXE unchanged.
                  opeu-glue      # port.h: the arena, the trace and the routine link the opening runs through.
                  opeu-services) # The OPEN-only assembly services (open_syslib, open_compos) and the FM driver.
