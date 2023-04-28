#pragma once

#include <nw4r/ut/ut_list.hh>

extern "C" {
#include <revolution.h>
}

namespace EGG {

class Disposer;

class Heap {
public:
    virtual void vf_00() = 0;
    virtual void vf_04() = 0;
    virtual void vf_08() = 0;
    virtual void vf_0c() = 0;
    virtual void initAllocator(void *allocator, s32 align) = 0;
    virtual void *alloc(u32 size, s32 align) = 0;
    virtual void free(void *block) = 0;
    virtual void destroy() = 0;
    virtual u32 resizeForMBlock(void *block, u32 size) = 0;
    virtual void vf_24() = 0;
    virtual void vf_28() = 0;

    static Heap *findContainHeap(const void *block);

    void appendDisposer(Disposer *disposer) {
        m_disposers.append(disposer);
    }
    void removeDisposer(Disposer *disposer) {
        m_disposers.remove(disposer);
    }

private:
    u8 _04[0x10 - 0x04];

public:
    MEMiHeapHead *heapHandle;

private:
    u8 _14[0x1c - 0x14];

public:
    u16 _1c;

private:
    u8 _1e[0x28 - 0x1e];
    nw4r::ut::List m_disposers;
    u8 _34[0x38 - 0x34];
};
static_assert(sizeof(Heap) == 0x38);

} // namespace EGG

void *operator new(size_t size, EGG::Heap *heap, int align);

void *operator new[](size_t size, int align);
void *operator new[](size_t size, EGG::Heap *heap, int align);
