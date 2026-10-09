/* io routines reached by address from the battle modules and stockord:
 *   io/waitkey.c 0a8e:004c delay_units (AL units)
 *   io/listui.c  0a8e:0140 table_columns (AL column, DL rows, BX first,
 *                          stack second, third)
 *   io/listdlg.c 0a8e:0bc0 list_dialog (AL limit, DL flags, BX window) -> AX
 *   io/sound.c   0d65:0032 finish_side_initialization (AL value),
 *                0d65:0060 music_stop (), 0d65:00fe set_turn_context (AL
 *                track), 0d65:01b0 music_command7 (), 0d65:02bc effect_finish ()
 *   io/soundcue.c 0d92:0008..0050: the battle handler table DS:b40e
 *                (btldraw/btlsprt dispatch), no arguments. */
#include "map/btl_link.h"

PORT_ROUTINE(opeu_delay_units)
{
    opeu_delay_units(ds, link_waitkey, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_table_columns)
{
    opeu_table_columns(ds, link_listui, (uint8_t)ARG(0), (uint8_t)ARG(1), ARG(2), ARG(3), ARG(4));
    return 0;
}

PORT_ROUTINE(opeu_list_dialog)
{
    return opeu_list_dialog(ds, link_listdlg, ARG(2), (uint8_t)ARG(0), (uint8_t)ARG(1));
}

PORT_ROUTINE(opeu_finish_side_initialization)
{
    opeu_finish_side_initialization(ds, link_sound, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_music_stop)
{
    (void)args; (void)count;
    opeu_music_stop(ds, link_sound);
    return 0;
}

PORT_ROUTINE(opeu_set_turn_context)
{
    opeu_set_turn_context(ds, link_sound, (uint8_t)ARG(0));
    return 0;
}

PORT_ROUTINE(opeu_music_command7)
{
    (void)args; (void)count;
    opeu_music_command7(ds, link_sound);
    return 0;
}

PORT_ROUTINE(opeu_effect_finish)
{
    (void)args; (void)count;
    opeu_effect_finish(ds, link_sound);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue0)
{
    (void)args; (void)count;
    opeu_sound_cue0(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue1)
{
    (void)args; (void)count;
    opeu_sound_cue1(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue2)
{
    (void)args; (void)count;
    opeu_sound_cue2(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue3)
{
    (void)args; (void)count;
    opeu_sound_cue3(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue4)
{
    (void)args; (void)count;
    opeu_sound_cue4(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue5)
{
    (void)args; (void)count;
    opeu_sound_cue5(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue6)
{
    (void)args; (void)count;
    opeu_sound_cue6(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue7)
{
    (void)args; (void)count;
    opeu_sound_cue7(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_control_begin)
{
    (void)args; (void)count;
    opeu_control_begin(ds, link_soundcue);
    return 0;
}

PORT_ROUTINE(opeu_sound_cue9)
{
    (void)args; (void)count;
    opeu_sound_cue9(ds, link_soundcue);
    return 0;
}
