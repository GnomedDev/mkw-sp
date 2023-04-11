#include <Common.h>

void SectionManager_init(void *self);

PATCH_S16(SectionManager_init, 0x8e, 0x540);
