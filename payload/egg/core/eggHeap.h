#pragma once

#include <Common.h>

typedef struct {
    u8 _00[0x10 - 0x00];
    void *heapHandle;
    u8 _14[0x1c - 0x14];
    u16 _1c;
    u8 _1e[0x38 - 0x1e];
} EGG_Heap;
static_assert(offsetof(EGG_Heap, _1c) == 0x1c);
static_assert(sizeof(EGG_Heap) == 0x38);

typedef struct {
    EGG_Heap base;
    // ...
} EGG_ExpHeap;

void EGG_ExpHeap_InitAlloc(EGG_ExpHeap *heap, void *alloc, int align);
