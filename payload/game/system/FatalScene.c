#include "FatalScene.h"

#include <Common.h>

void **GetSpAllocAddr() {
    extern void *lyt_Layout_DT(void *, s32);

    const u32 ppc_lwz = *(const u32 *)(((const char *)&lyt_Layout_DT) + 0x48);

    assert((ppc_lwz >> (32 - 6)) == 32);
    assert(((ppc_lwz << 6) >> (32 - 5)) == 3);
    assert(((ppc_lwz << (6 + 5)) >> (32 - 5)) == 13);
    register s32 r13_offset = (s16)(ppc_lwz & 0xffff);

    void **result;
    asm("add %0, %1, 13" : "=r"(result) : "r"(r13_offset));
    return result;
}
