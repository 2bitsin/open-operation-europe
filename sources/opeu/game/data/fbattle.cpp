#include "data/fbattle.hpp"
/* Far data segment 384f (image 0x384f0..0x38730): battle report formats.
 * DS:50fa..5119 holds eight far pointers to these strings (the last repeated).
 */
/* The module's far objects lie in declaration order, without padding, in one paragraph-aligned segment. */

/* +0000 */ char opeu_battle_report_0[] =
    "On %d/%2d, enemy Commander %s was killed in battle. His army was routed.";
/* +0049 */ char opeu_battle_report_1[] =
    "On %d/%2d, Commander %s scored a sweeping victory. The enemy\n"
    "fled the area and a formidable line of defense was established.";
/* +00c6 */ char opeu_battle_report_2[] =
    "On %d/%2d, Commander %s pulled off a victory. Though the\n"
    "military gain was small, the victory was considered a forward step.";
/* +0143 */ char opeu_battle_report_3[] =
    "On %d/%2d, Commander %s won by a narrow margin. Though it\n"
    "was a victory, we needed time to bring the area under control.";
/* +01bc */ char opeu_battle_report_4[] =
    "On %d/%2d, Commander %s was killed in action.";
/* +01ea */ char opeu_battle_report_5[] =
    "On %d/%2d, Commander %s's army was annihilated.";
/* +021a */ char opeu_battle_report_6[] =
    "On %d/%2d, Commander %s was defeated.";
