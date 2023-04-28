#pragma once

#include <Common.h>

typedef struct {
    void *vtable;
    u8 _04[0x10 - 0x04];
    struct lyt_Pane *rootPane;
    u8 _14[0x18 - 0x14];
    float width;
    float height;
} lyt_Layout;
static_assert(sizeof(lyt_Layout) == 0x20);

lyt_Layout *lyt_Layout_CT(lyt_Layout *layout);
void lyt_Layout_DT(lyt_Layout *layout, int type);

typedef struct {
    u32 _[4];
} MEMAllocator;
extern MEMAllocator *lyt_spAlloc;
