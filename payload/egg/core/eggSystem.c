#include "eggSystem.h"

#include <game/system/Console.h>
#include <sp/Commands.h>
#include <sp/ItemCommand.h>
#include <sp/keyboard/Keyboard.h>

PATCH_B(EGG_ProcessMeter_draw + 0xa4, Console_draw);
