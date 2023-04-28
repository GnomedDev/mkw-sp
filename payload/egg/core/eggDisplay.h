#pragma once

#include <Common.h>

typedef struct {
    u8 flag;
    u8 _[3];
    struct EGGDisplay_Vtable *vt;
} EGGDisplay;

typedef struct EGGDisplay_Vtable {
    u32 _[2];
    void (*beginFrame)(EGGDisplay *);
    void (*beginRender)(EGGDisplay *);
    void (*endRender)(EGGDisplay *);
    void (*endFrame)(EGGDisplay *);
} EGGDisplay_Vtable;
