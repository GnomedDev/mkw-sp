#include "eggHeap.hh"

extern "C" void *malloc(u32 size) {
    return new u8[size];
}

extern "C" void free(void *memBlock) {
    return delete reinterpret_cast<u8 *>(memBlock);
}
