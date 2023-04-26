#include "eggSystem.h"

#include <game/system/Console.h>

PATCH_B(EGG_ProcessMeter_draw + 0xa4, Console_draw);
