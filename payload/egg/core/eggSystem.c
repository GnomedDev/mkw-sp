#include <game/system/Console.h>

#include <Common.h>

void EGG_ProcessMeter_draw(void *);

PATCH_B(EGG_ProcessMeter_draw + 0xa4, Console_draw);
